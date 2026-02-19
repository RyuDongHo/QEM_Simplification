# QEM Simplification

**Quadric Error Metric (QEM)** 기반 메시 단순화 소프트웨어입니다.

## 프로젝트 소개

이 프로젝트는 QEM(Quadric Error Metric) 알고리즘을 적용하여 3D 메시의 폴리곤 수를 줄이면서도 원본 형상을 최대한 유지하는 메시 단순화(Mesh Simplification) 기능을 구현합니다.

## 기술 스택

- **언어**: C++17
- **빌드 시스템**: CMake
- **그래픽 라이브러리**: 
  - OpenGL 3.3
  - GLEW 2.3.1
  - GLFW 3.4
  - GLM (Mathematics Library)

## 프로젝트 구조

```
simplification/
├── lib/                    # 외부 라이브러리
│   ├── glew/              # GLEW 라이브러리
│   ├── glfw/              # GLFW 라이브러리
│   └── glm/               # GLM 수학 라이브러리
├── includes/              # 헤더 파일
│   └── common.h           # 공통 유틸리티 헤더
├── src/                   # 소스 파일
│   ├── main.cpp           # 메인 프로그램
│   └── common.cpp         # 공통 유틸리티 구현
├── shader/                # GLSL 셰이더
│   ├── vertex.glsl        # 버텍스 셰이더
│   └── fragment.glsl      # 프래그먼트 셰이더
└── CMakeLists.txt         # CMake 빌드 설정
```

## 빌드 방법

### Windows (Visual Studio)

```bash
# 빌드 디렉토리 생성
mkdir build
cd build

# CMake 구성
cmake ..

# 빌드
cmake --build . --config Release
```

### 실행

```bash
cd build/Release
./QEM_Simplification.exe
```

## 기능

- [x] OpenGL 기반 3D 렌더링 파이프라인 구축
- [x] 기본 VAO/VBO 설정 및 메시 렌더링
- [x] Shader 시스템 구축 (vertex/fragment shader)
- [x] OBJ 파일 로더 구현 (vertices, UVs, normals 지원)
- [x] 텍스처 로딩 및 매핑 (stb_image 사용)
- [x] 트랙볼 카메라 컨트롤 (마우스 드래그로 회전)
- [ ] QEM 알고리즘 구현
- [ ] Edge Collapse 연산
- [ ] 메시 단순화 파라미터 조절 UI

## 참고 문헌

- Garland, M., & Heckbert, P. S. (1997). "Surface simplification using quadric error metrics." *SIGGRAPH 97*.
