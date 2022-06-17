#include "ofApp.h"
#include <vector> // calcTangents() 함수에서 동적 배열인 std::vector 컨테이너 클래스 템플릿을 사용하기 위해 해당 클래스를 include 시킴.

// 탄젠트 벡터 계산 후, 메쉬의 버텍스 컬러 데이터에 임시로 저장해두는 함수
void calcTangents(ofMesh& mesh) { // mesh 함수인자는 'ofMesh' 타입을 참조한다는 뜻. (diffuse-lighting 레포지토리 참조자 관련 설명 참고)
    using namespace glm;
    std::vector<vec4> tangents; // c++ 에 정의된 표준 라이브러리 사용 시, std:: 접두어를 항상 붙여주도록 함. 여기서는 동적 배열인 std::vector 컨테이너 클래스 템플릿을 사용함.
    tangents.resize(mesh.getNumVertices()); // 버텍스 개수만큼 탄젠트 벡터를 담는 동적 배열의 길이 생성
    
    uint indexCount = mesh.getNumIndices(); // 버텍스의 인덱스 개수만큼 인덱스 카운트 개수 지정
    
    const vec3* vertices = mesh.getVerticesPointer();
    const vec2* uvs = mesh.getTexCoordsPointer();
    const uint* indices = mesh.getIndexPointer();
    
    for (uint i = 0; i < indexCount - 2; i += 3) {
        // 3개의 인접한 버텍스들 -> 즉, 하나의 삼각형 안의 3개의 버텍스들의 위치데이터와 uv데이터를 각각 구함
        // 왜 삼각형일까? 삼각형만이 온전한 평면을 구성하고, 같은 평면안에 존재한다면 탄젠트 공간 내의 탄젠트 벡터도 동일한 걸 공유한다고 가정하기 때문인 것 같음.
        // 왜냐, 같은 평면 안에 존재하는 버텍스라면, 그 버텍스들은 동일한 노말벡터를 공유하기 때문에, 탄젠트 벡터도 동일하다고 봄.
        const vec3& v0 = vertices[indices[i]];
        const vec3& v1 = vertices[indices[i + 1]];
        const vec3& v2 = vertices[indices[i + 2]];
        const vec2& uv0 = uvs[indices[i]];
        const vec2& uv1 = uvs[indices[i + 1]];
        const vec2& uv2 = uvs[indices[i + 2]];
        
        // v0 버텍스에 연결된 2개의 선분(벡터)을 구함 -> 3차원 공간 상의 선분(벡터)
        vec3 edge1 = v1 - v0; // 선분(벡터)1
        vec3 edge2 = v2 - v0; // 선분(벡터)2
        // v0 버텍스의 uv좌표에 연결된 uv좌표계 상의 2개의 선분(벡터)도 구함 (아마 distance UV 의 줄임말인 듯.) -> 2차원 공간(uv좌표계) 상의 선분(벡터)
        vec2 dUV1 = uv1 - uv0; // uv좌표계 선분1
        vec2 dUV2 = uv2 - uv0; // uv좌표계 선분2
        
        // 위에서 구한 두 벡터를 기저 벡터로 삼아(?) 탄젠트 벡터를 구하는 과정같음.. (아직 자세히는 모르겠음ㅜ)
        float f = 1.0f / (dUV1.x * dUV2.y - dUV2.x * dUV1.y);
        
        vec4 tan;
        tan.x = f * (dUV2.y * edge1.x - dUV1.y * edge2.x);
        tan.y = f * (dUV2.y * edge1.y - dUV1.y * edge2.y);
        tan.z = f * (dUV2.y * edge1.z - dUV1.y * edge2.z);
        tan.w = 0;
        tan = normalize(tan); // 계산한 탄젠트 벡터를 정규화하여 길이를 1로 맞춤.
        
        // 서로 인접하는 3개의 버텍스, 즉 한 삼각형 안의 버텍스들은 모두 동일한 탄젠트 벡터를 갖도록
        // tangents 동적배열에 인접한 3개의 인덱스에 동일한 탄젠트 벡터를 넣어줌..
        tangents[indices[i]] += (tan);
        tangents[indices[i + 1]] += (tan);
        tangents[indices[i + 2]] += (tan);
    }
    
    int numColors = mesh.getNumColors(); // 오픈프레임웍스가 버텍스 탄젠트 데이터를 지원하지 않아서 버텍스 컬러데이터에 대신 넣어주려는 것.

    for (int i = 0; i < tangents.size(); ++i) {
        vec3 t = normalize(tangents[i]); // 위에 반복문에서 저장해 둔 각 인덱스의 탄젠트 벡터를 가져온 뒤, vec3에 할당함으로써 각 탄젠트벡터의 x, y, z 컴포넌트만 가져옴.
        
        // 탄젠트벡터 데이터가 컬러데이터와 개수가 안맞을 수 있기 때문에,
        // i가 전체 컬러데이터 수보다 적으면 기존 i번쩨 컬러데이터를 setColor() 로 탄젠트 벡터 데이터로 덮어쓰고,
        // i가 전체 컬러데이터 수보다 크다면, addColor() 로 탄젠트 벡터 데이터를 전달해서 컬러데이터를 새로 추가함.
        if (i >= numColors) {
            mesh.addColor(ofFloatColor(t.x, t.y, t.z, 0.0));
        } else {
            mesh.setColor(i, ofFloatColor(t.x, t.y, t.z, 0.0));
        }
    }
}

// 조명계산 최적화를 위해, 쉐이더에서 반복계산하지 않도록, c++ 에서 한번만 계산해줘도 되는 작업들을 수행하는 보조함수들
glm::vec3 getLightDirection(DirectionalLight& l) {
    // 조명벡터 direction에 -1을 곱해서 조명벡터의 방향을 뒤집어주고, 셰이더에서 내적계산을 해주기 위해 길이를 1로 정규화해서 맞춰줌.
    return glm::normalize(l.direction * -1.0f);
}

glm::vec3 getLightColor(DirectionalLight& l) {
    // vec3 값인 조명색상에 float 값인 조명강도를 스칼라배로 곱해줘서 조명색상의 밝기를 지정함.
    return l.color * l.intensity;
}

//--------------------------------------------------------------
void ofApp::setup(){
    ofDisableArbTex(); // 스크린 픽셀 좌표를 사용하는 텍스쳐 관련 오픈프레임웍스 레거시 지원 설정 비활성화. (uv좌표계랑 다르니까!)
    ofEnableDepthTest(); // 깊이테스트를 활성화하여 z버퍼에 저장해서 각 요소에서 카메라와의 거리를 기준으로 앞뒤를 구분하여 렌더링할 수 있도록 함.
    
    // 이번에는 ofApp 맨 처음 설정에서 shieldMesh 를 바라보기 적당한 카메라 위치와 시야각을 지정함.
    cam.pos = glm::vec3(0, 0.75f, 1.0f); // 카메라 위치는 z축으로 1만큼 안쪽으로 들어가게 하고, 조명 연산 결과를 확인하기 위해 y축으로도 살짝 올려줌
    cam.fov = glm::radians(90.0f); // 원근 프러스텀의 시야각은 일반 PC 게임에서는 90도 전후의 값을 사용함. -> 라디안 각도로 변환하는 glm 내장함수 radians() 를 사용함.
    
    planeMesh.load("plane.ply"); // planeMesh 메쉬로 사용할 모델링 파일 로드
    calcTangents(planeMesh); // plane 메쉬의 버텍스들을 이용해서 각 버텍스의 탄젠트 벡터를 구한 뒤 버텍스 컬러데이터 자리에 저장하는 함수 실행
    
    shieldMesh.load("shield.ply"); // shieldMesh 메쉬로 사용할 모델링 파일 로드
    calcTangents(shieldMesh); // shield 메쉬에 탄젠트 벡터를 구한 뒤 버텍스 컬러 자리에 저장하는 함수 실행

    waterShader.load("water.vert", "water.frag"); // planeMesh 에 노말맵을 활용한 물셰이더를 적용하기 위한 셰이더 파일 로드
    blinnPhong.load("mesh.vert", "blinn-phong.frag"); // shieldMesh 에 (노말맵)텍스쳐를 활용한 Blinn-phong 반사모델을 적용하기 위한 셰이더 파일 로드
    
    waterNrm.load("water_nrm.png"); // planeMesh 의 조명계산에서 노말맵으로 사용할 텍스쳐 로드
    waterNrm.getTexture().setTextureWrap(GL_REPEAT, GL_REPEAT); // 프래그먼트 셰이더에서 노말맵 텍스쳐를 타일링하여 샘플링할 것이므로, 노말맵의 랩 모드를 반복으로 지정함.
    
    diffuseTex.load("shield_diffuse.png"); // shieldMesh 의 조명계산에서 디퓨즈 라이팅 계산에 사용할 텍스쳐 로드
    specTex.load("shield_spec.png"); // shieldMesh 의 조명계산에서 스펙큘러 라이팅 계산에 사용할 텍스쳐 로드
    nrmTex.load("shield_normal.png"); // shieldMesh 의 조명계산에서 노말맵으로 사용할 텍스쳐 로드
}

//--------------------------------------------------------------
void ofApp::update(){

}

// waterMesh 의 각종 변환행렬을 계산한 뒤, 유니폼 변수들을 전송해주면서 드로우콜을 호출하는 함수
void ofApp::drawWater(DirectionalLight& dirLight, glm::mat4& proj, glm::mat4& view) {
    using namespace glm;
    
    static float t = 0.0f; // static 을 특정 함수 내에서 사용하는 것을 '정적 지역 변수'라고 하며, 이 할당문은 drawWater() 함수 최초 호출 시 1번만 실행됨.
    t += ofGetLastFrameTime(); // 이전 프레임과 현재 프레임의 시간 간격인 '델타타임'을 리턴받는 함수를 호출해서 유니폼 변수로 전송할 시간값 t에 매 프레임마다 더해줌.
    
    // waterMesh 의 모델행렬 계산 (회전행렬 및 크기행렬만 적용)
    vec3 right = vec3(1, 0, 0); // waterMesh 모델의 회전행렬을 계산할 시, x축 방향으로만 회전할 수 있도록 회전축 벡터를 구해놓음.
    mat4 rotation = rotate(radians(-90.0f), right); // waterMesh 회전행렬은 x축 기준으로 cAngle(-90도) 회전시킴.
    mat4 model = rotation * scale(vec3(5.0, 4.0, 4.0)); // 열 우선 행렬이므로, 원하는 행렬 곱셈과 반대순서인 회전행렬 * 크기행렬 순으로 곱해줌
    
    // 최적화를 위해 c++ 단에서 투영 * 뷰 * 모델행렬을 한꺼번에 곱해서 버텍스 셰이더에 전송함.
    mat4 mvp = proj * view * model; // 열 우선 행렬이라 원래의 곱셈 순서인 '모델 -> 뷰 -> 투영'의 반대 순서로 곱해줘야 함.
    
    /**
         모델의 버텍스가 갖고있는 기본 노말벡터는 오브젝트공간을 기준으로 되어있음.
         그러나, 조명계산을 하려면 이러한 노말벡터를 월드공간으로 변환해야 함.
                 
         그럼 노말벡터도 그냥 모델행렬인 mat4 model 을 버텍스 셰이더에서 곱해주면 되는거 아닌가?
         이렇게 할 수도 있지만, 만약 모델행렬에 '비일률적 크기행렬' (예를들어, (0.5, 1.0, 1.0) 이런거)
         가 적용되어 있다면, 특정 축마다 scale 이 다르게 늘어나는 과정에서 노말벡터의 방향이 휘어버리게 됨. -> p.190 참고
                 
         이걸 똑바르게 세워주려면, '노멀행렬' 이라는 새로운 행렬이 필요함.
         노말행렬은 '모델행렬의 상단 3*3 역행렬의 전치행렬' 로 정의할 수 있음.
                 
         역행렬, 전치행렬, 상단 3*3 행렬에 대한 각각의 개념은 위키백과, 구글링, 북마크한거 참고...
                 
         어쨋든 위의 정의에 따라 아래와 같이 노말행렬을 구하고, 버텍스 셰이더로 쏴주면 됨.
    */
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    
    ofShader& shd = waterShader; // 참조자 shd 는 ofShader 타입의 멤버변수 waterShader 를 참조하도록 함.
    
    // shd(waterShader 를 참조) 를 바인딩하여 사용 시작
    shd.begin();
    shd.setUniformMatrix4f("mvp", mvp); // 위에서 한꺼번에 합쳐준 mvp 행렬을 버텍스 셰이더 유니폼 변수로 전송
    shd.setUniformMatrix4f("model", model); // 버텍스 좌표를 월드좌표로 변환하기 위해 모델행렬만 따로 버텍스 셰이더 유니폼 변수로 전송
    shd.setUniformMatrix3f("normalMatrix", normalMatrix); // 노말행렬을 버텍스 셰이더 유니폼 변수로 전송
    shd.setUniform3f("meshSpecCol", glm::vec3(1, 1, 1)); // 스펙큘러 색상을 흰색으로 지정하여 유니폼 변수로 전송
    shd.setUniformTexture("normTex", waterNrm, 0); // 노말 매핑에 사용할 텍스쳐 유니폼 변수로 전송
    shd.setUniform1f("time", t); // uv 스크롤링에 사용할 시간값 유니폼 변수로 전송
    
    shd.setUniform3f("ambientCol", glm::vec3(0.1, 0.1, 0.1)); // 환경광으로 사용할 앰비언트 라이트 색상값을 유니폼 변수로 전송.
    shd.setUniform3f("lightDir", getLightDirection(dirLight)); // 조명벡터를 음수화하여 뒤집어주고, 다시 정규화하여 길이를 1로 맞춘 뒤, 유니폼 변수로 전송
    shd.setUniform3f("lightCol", getLightColor(dirLight)); // 조명색상을 조명강도와 곱해준 뒤, 유니폼 변수로 전송
    shd.setUniform3f("cameraPos", cam.pos); // 프래그먼트 셰이더에서 뷰 벡터를 계산하기 위해 카메라 좌표(카메라 월드좌표)를 프래그먼트 셰이더 유니폼 변수로 전송
    
    planeMesh.draw(); // planeMesh(waterMesh) 메쉬 드로우콜 호출하여 그려줌.
    
    shd.end();
    // shd(waterShader) 사용 중단
}

void ofApp::drawShield(DirectionalLight& dirLight, glm::mat4& proj, glm::mat4& view) {
    using namespace glm;
    
    mat4 model = translate(vec3(0.0, 0.75, 0.0f)); // shieldMesh 의 모델행렬 계산 (이동행렬만 적용)
    mat4 mvp = proj * view * model; // 최적화를 위해 c++ 단에서 투영 * 뷰 * 모델행렬을 한꺼번에 곱해서 버텍스 셰이더에 전송함.
    mat3 normalMatrix = mat3(transpose(inverse(model))); // 노말행렬은 '모델행렬의 상단 3*3 역행렬의 전치행렬' 로 계산함.
    
    ofShader& shd = blinnPhong; // 참조자 shd 는 ofShader 타입의 멤버변수 blinnPhong 셰이더 객체를 참조하도록 함.
    
    // shd(blinnPhong 를 참조) 를 바인딩하여 사용 시작
    shd.begin();
    
    shd.setUniformMatrix4f("mvp", mvp); // 위에서 한꺼번에 합쳐준 mvp 행렬을 버텍스 셰이더 유니폼 변수로 전송
    shd.setUniformMatrix4f("model", model); // 버텍스 좌표를 월드좌표로 변환하기 위해 모델행렬만 따로 버텍스 셰이더 유니폼 변수로 전송
    shd.setUniformMatrix3f("normalMatrix", normalMatrix); // 노말행렬을 버텍스 셰이더 유니폼 변수로 전송
    shd.setUniform3f("meshSpecCol", glm::vec3(1, 1, 1)); // 스펙큘러 색상을 흰색으로 지정하여 유니폼 변수로 전송
    shd.setUniformTexture("diffuseTex", diffuseTex, 0); // 디퓨즈 라이팅 계산에 사용할 텍스쳐 유니폼 변수로 전송
    shd.setUniformTexture("specTex", specTex, 1); // 스펙큘러 라이팅 계산에 사용할 텍스쳐 유니폼 변수로 전송
    shd.setUniformTexture("nrmTex", nrmTex, 2); // 노말 매핑에 사용할 텍스쳐 유니폼 변수로 전송
    
    shd.setUniform3f("ambientCol", glm::vec3(0.1, 0.1, 0.1)); // 배경색과 동일한 앰비언트 라이트 색상값을 유니폼 변수로 전송.
    shd.setUniform3f("lightDir", getLightDirection(dirLight)); // 조명벡터를 음수화하여 뒤집어주고, 다시 정규화하여 길이를 1로 맞춘 뒤, 유니폼 변수로 전송
    shd.setUniform3f("lightCol", getLightColor(dirLight)); // 조명색상을 조명강도와 곱해준 뒤, 유니폼 변수로 전송
    shd.setUniform3f("cameraPos", cam.pos); // 프래그먼트 셰이더에서 뷰 벡터를 계산하기 위해 카메라 좌표(카메라 월드좌표)를 프래그먼트 셰이더 유니폼 변수로 전송
    
    shieldMesh.draw(); // shieldMesh 메쉬 드로우콜 호출하여 그려줌.
    
    shd.end();
    // shd(blinnPhong) 사용 중단
}

//--------------------------------------------------------------
void ofApp::draw(){
    using namespace glm; // 이제부터 현재 블록 내에서 glm 라이브러리에서 꺼내 쓸 함수 및 객체들은 'glm::' 을 생략해서 사용해도 됨.
    
    // 조명구조체 dirLight 에 조명데이터를 할당해 줌.
    DirectionalLight dirLight; // 조명데이터 구조체인 DirectionLight 타입의 객체 변수 dirLight 선언
    dirLight.direction = normalize(vec3(0.5, -1, -1)); // 조명벡터 방향 지정
    dirLight.color = vec3(1, 1, 1); // 조명색상은 흰색으로 지정
    dirLight.intensity = 1.0f; // 조명강도도 1로 지정. 참고로, 1보다 큰값으로 조명강도를 조명색상에 곱해줘봤자, 프래그먼트 셰이더는 (1, 1, 1, 1) 이상의 색상값을 처리할 수 없음.

    // 물 셰이더에서 스펙큘러를 더 잘 보이게 하기 위해, 조명방향만 변경해 준 조명데이터를 따로 만듦.
    DirectionalLight waterLight;
    waterLight.direction = normalize(vec3(0.5, -1, 1)); // 조명벡터 방향만 바꿈
    waterLight.color = vec3(1, 1, 1);
    waterLight.intensity = 1.0f;
    
    // 투영행렬 계산
    float aspect = 1024.0f / 768.0f; // main.cpp 에서 정의한 윈도우 실행창 사이즈를 기준으로 원근투영행렬의 종횡비(aspect)값을 계산함.
    mat4 proj = perspective(cam.fov, aspect, 0.01f, 10.0f); // glm::perspective() 내장함수를 사용해 원근투영행렬 계산.
    
    // 카메라 변환시키는 뷰행렬 계산. 이동행렬만 적용
    mat4 view = inverse(translate(cam.pos)); // 뷰행렬은 카메라 움직임에 반대방향으로 나머지 대상들을 움직이는 변환행렬이므로, glm::inverse() 내장함수로 역행렬을 구해야 함.
    
    // 이후의 연산은 shield 메쉬 드로우 함수와 water 메쉬 드로우 함수로 쪼개서 추출함.
    drawShield(dirLight, proj, view);
    drawWater(waterLight, proj, view);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
