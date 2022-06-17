#version 410

// c++ 미리 계산된 후 받아온 조명연산에 필요한 유니폼 변수들
uniform vec3 lightDir; // 뒤집어지고 정규화된 조명벡터
uniform vec3 lightCol; // 조명색상 (조명강도가 곱해짐)
uniform vec3 cameraPos; // 각 프래그먼트 -> 카메라 방향의 벡터 (이하 '뷰 벡터' 또는 '카메라 벡터') 계산에 필요한 카메라 월드공간 좌표
uniform vec3 ambientCol; // 앰비언트 라이트(환경광 또는 글로벌 조명(전역 조명))의 색상 

// 물 셰이더를 만들기 위해 필요한 노말맵을 전달받는 유니폰 변수들
uniform sampler2D normTex;
uniform sampler2D normTex2; // 이 예제에서는 샘플링할 uv좌표를 달리 계산해서 서로 다른 노멀(벡터)를 구하므로, 두 번째 노멀맵이 따로 필요하진 않음.

in vec3 fragNrm; // 버텍스 셰이더에서 받아온 물 표면 모델의 (월드공간) 노멀벡터가 보간되어 들어온 값
in vec3 fragWorldPos; // 버텍스 셰이더에서 받아온 물 표면 모델의 월드공간 위치 좌표가 보간되어 들어온 값

// 각 버텍스마다 서로 다르게 계산된 uv좌표가 보간되어 들어온 값 (얘내들로 노말맵을 샘플링하므로, 서로 다른 탄젠트 공간 노멀벡터를 샘플링해올 것임)
in vec2 fragUV;
in vec2 fragUV2;

in mat3 TBN; // 노말맵 텍스쳐에서 샘플링한 탄젠트 공간의 노말벡터를 월드공간으로 변환하기 위해, 버텍스 셰이더에서 각 버텍스마다 계산한 뒤, 보간되어 들어온 TBN 행렬

out vec4 outCol; // 최종 출력할 색상을 계산하여 다음 파이프라인으로 넘겨줄 변수

void main(){
  // 아래의 normal, normal2 는 동일한 텍스쳐를 사용하지만,
  // 샘플링하는 uv좌표가 다르므로, 결과적으로 서로 다른 (탄젠트 공간의)노말벡터를 리턴받음.
  vec3 normal = texture(normTex, fragUV).rgb;
  normal = (normal * 2.0 - 1.0); // 노말맵에서 샘플링한 텍셀값 범위 0 ~ 1 을 탄젠트 공간의 노말벡터가 속한 좌표계 범위인 -1 ~ 1 로 맵핑함.

  vec3 normal2 = texture(normTex, fragUV2).rgb;
  normal2 = (normal2 * 2.0 - 1.0); // 노말맵에서 샘플링한 텍셀값 범위 0 ~ 1 을 탄젠트 공간의 노말벡터가 속한 좌표계 범위인 -1 ~ 1 로 맵핑함.

  normal = normalize(TBN * (normal + normal2)); // 두 탄젠트 공간의 노멀벡터를 더해서 그 사이의 하프벡터를 구하고, 그걸 TBN 행렬과 곱해 월드공간의 노말벡터로 변환함.
  // 여기까지 해야 노말맵에서 샘플링해온 노말벡터는 조명계산에 써먹을 수 있는 상태가 되었고, 이후의 계산은 원래 하던 blinn-phong 계산과 동일하게 수행하면 됨.

  vec3 viewDir = normalize(cameraPos - fragWorldPos); // 카메라의 월드공간 좌표 - 각 프래그먼트 월드공간 좌표를 빼서 각 프래그먼트 -> 카메라 방향의 벡터인 뷰 벡터 계산

  // Blinn-Phong 공식에서의 스펙큘러 라이팅 계산
  vec3 halfVec = normalize(viewDir + lightDir); // 뷰 벡터와 조명벡터 사이의 하프벡터를 구함.
  float specAmt = max(0.0, dot(halfVec, normal)); // 하프벡터와 노멀벡터의 내적값을 구한 뒤, max() 함수로 음수값 제거
  float specBright = pow(specAmt, 512.0); // 물 재질은 거울처럼 반사가 아주 세므로, 스펙큘러 계산 시 광택지수를 512 처럼 높게 잡아줘야 함.
  vec3 specCol = lightCol * specBright; // 물 셰이더에서는 스펙큘러 텍스쳐는 따로 사용 안하므로, texture(specTex, fragUV).X 뭐 이런 스펙큘러맵 샘플링값은 안곱해줘도 됨.

  // 디퓨즈 라이팅 계산 (노멀벡터와 조명벡터를 내적)
  float diffAmt = max(0.0, dot(normal, lightDir)); // 정규화된 노멀벡터와 조명벡터의 내적값을 구한 뒤, max() 함수로 음수인 내적값 제거.
  vec3 diffCol = vec3(0.3, 0.3, 0.4) * lightCol * diffAmt; // 디퓨즈 텍스쳐에서 샘플링한 값은 사용하지 않고, 물체의 원 색상값인 파란색을 곱해줌.

  outCol = vec4(diffCol + specCol + ambientCol, 1.0);
}