#version 410

// c++ 미리 계산된 후 받아온 조명연산에 필요한 유니폼 변수들
uniform vec3 lightDir; // 뒤집어지고 정규화된 조명벡터
uniform vec3 lightCol; // 조명색상 (조명강도가 곱해짐)
uniform vec3 cameraPos; // 각 프래그먼트 -> 카메라 방향의 벡터 (이하 '뷰 벡터' 또는 '카메라 벡터') 계산에 필요한 카메라 월드공간 좌표
uniform vec3 ambientCol; // 앰비언트 라이트(환경광 또는 글로벌 조명(전역 조명))의 색상 
uniform sampler2D diffuseTex; // 디퓨즈 라이팅 계산에 사용할 텍스쳐를 담는 변수
uniform sampler2D specTex; // 스펙큘러 라이팅 계산에 사용할 텍스쳐를 담는 변수
uniform sampler2D nrmTex; // 노말 매핑에 사용할 노말맵 텍스쳐를 담는 변수

in vec3 fragNrm; // 버텍스 셰이더에서 받아온 shield 모델의 (월드공간) 노멀벡터가 보간되어 들어온 값
in vec3 fragWorldPos; // 버텍스 셰이더에서 받아온 shield 모델의 월드공간 위치 좌표가 보간되어 들어온 값
in vec2 fragUV; // 라이팅 계산에 사용할 텍스쳐들을 샘플링하기 위해 shield 모델의 버텍스의 uv좌표들을 보간하여 들어온 값
in mat3 TBN; // 노말맵 텍스쳐에서 샘플링한 탄젠트 공간의 노말벡터를 월드공간으로 변환하기 위해, 버텍스 셰이더에서 각 버텍스마다 계산한 뒤, 보간되어 들어온 TBN 행렬

out vec4 outCol; // 최종 출력할 색상을 계산하여 다음 파이프라인으로 넘겨줄 변수

void main(){
  // vec3 normal = normalize(fragNrm); // 프래그먼트 셰이더에서 보간된 노멀벡터는 길이가 1로 보존되지 않으므로, 연산에 사용하기 전 다시 정규화해줘야 함.
  // 버텍스 셰이더에서 받아온 노말벡터(fragNrm)를 보간하지 않고, 노말맵 텍스쳐에서 샘플링한 노말벡터를 TBN 행렬로 곱해 월드공간으로 변환한 후 사용할 것임.
  vec3 normal = texture(nrmTex, fragUV).rgb; // 노말맵 텍스쳐에서 텍셀값을 샘플링한 뒤, vec3 노말벡터 자리에 할당함.

  // 위의 샘플링한 노말벡터는 어디까지나 텍스쳐의 텍셀값, 즉 색상값이므로 범위가 0 ~ 1까지 밖에 표현이 안됨. 
  // 그런데, 탄젠트 공간의 노말벡터는 실제로 -1 ~ 1 사이의 정규화된 좌표계를 사용하고 있고, 음의 방향으로도 벡터를 표현할 수 있어야 하기 때문에
  // 0 ~ 1 사이의 컴포넌트 범위를 -1 ~ 1 사이로 맵핑한 뒤 정규화한 것.
  normal = normalize(normal * 2.0 - 1.0);
  normal = normalize(TBN * normal); // 컴포넌트의 값 범위를 바로잡은 노말벡터를 TBN 행렬과 곱해줌으로써, 탄젠트공간 -> 월드공간 으로 변환을 수행한 뒤, 길이를 1로 다시 정규화함.
  // 여기까지 해야 노말맵에서 샘플링해온 노말벡터는 조명계산에 써먹을 수 있는 상태가 되었고, 이후의 계산은 원래 하던 blinn-phong 계산과 동일하게 수행하면 됨.

  vec3 viewDir = normalize(cameraPos - fragWorldPos); // 카메라의 월드공간 좌표 - 각 프래그먼트 월드공간 좌표를 빼서 각 프래그먼트 -> 카메라 방향의 벡터인 뷰 벡터 계산

  // Blinn-Phong 공식에서의 스펙큘러 라이팅 계산
  vec3 halfVec = normalize(viewDir + lightDir); // 뷰 벡터와 조명벡터 사이의 하프벡터를 구함
  float specAmt = max(0.0, dot(halfVec, normal)); // 하프벡터와 노멀벡터의 내적값을 구한 뒤, max() 함수로 음수값 제거
  float specBright = pow(specAmt, 2.0); // 퐁 반사모델에서와 동일한 스펙큘러 하이라이트를 얻으려면, 퐁 반사모델에서 사용했던 광택값의 2~4배 값을 거듭제곱해야 함. 따라서 0.5의 4배인 2를 광택값으로 사용함.
  vec3 specCol = texture(specTex, fragUV).x * lightCol * specBright; // c++ 에서 전달해준 스펙큘러 하이라이트 색상 대신, 스펙큘러 맵에서 샘플링한 텍셀값을 사용할거임. 
  // 스펙큘러 맵은 흑백이므로, 텍셀값의 r채널 하나만으로 스칼라배를 해줘도 무방함.

  // 디퓨즈 라이팅 계산 (노멀벡터와 조명벡터를 내적)
  float diffAmt = max(0.0, dot(normal, lightDir)); // 정규화된 노멀벡터와 조명벡터의 내적값을 구한 뒤, max() 함수로 음수인 내적값 제거.
  vec3 meshCol = texture(diffuseTex, fragUV).xyz; // 물체의 원색상은 c++ 에서 전달해준 색상값 대신, 디퓨즈 텍스쳐에서 샘플링한 텍셀값을 사용할거임.
  vec3 diffCol = meshCol * lightCol * diffAmt; // '물체의 원색상 * 조명색상 * 디퓨즈 라이트값' 을 곱해 디퓨즈 라이트 색상값 결정

  // 앰비언트 라이트 계산 (앰비언트 라이트 색상값과 물체의 원색상을 곱함)
  vec3 ambient = ambientCol * meshCol; // 물체의 원 색상과 다른 쌩뚱맞은 색이 나오면 안되어서 물체의 원 색상을 곱해주는 것

  outCol = vec4(diffCol + specCol + ambient, 1.0); // '스펙큘러 라이트 색상값 + 디퓨즈 라이트 색상값 + 앰비언트 라이트 색상값(물체의 원색상 반영됨)' 을 합쳐서 최종 색상값 결정
}

/*
  texture()

  원래 glsl 내장함수로 텍스쳐 샘플링할 때
  texture2D() 함수를 사용했었는데,
  현재 410 버전에서 사용하면 에러가 남.

  아무래도 410 버전의 glsl 은 문법이 변경된 거 같음.
*/
