# QEM Simplification

**Quadric Error Metric (QEM)** 기반 메시 단순화 소프트웨어입니다.

## 프로젝트 소개

이 프로젝트는 QEM(Quadric Error Metric) 알고리즘을 적용하여 3D 메시의 폴리곤 수를 줄이면서도 원본 형상을 최대한 유지하는 메시 단순화(Mesh Simplification) 기능을 구현합니다.

## 시연 영상

![QEM Simplification Demo](test.gif)

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

## 사용 방법

프로그램 실행 중 다음 키를 사용할 수 있습니다:

- **J 키**: FOV 값 감소 (줌 인)
- **K 키**: FOV 값 증가 (줌 아웃)
- **Spacebar**: 단계별로 메시 단순화 진행

## 테스트용 메시 추가 방법

프로젝트 루트의 `resource/` 폴더에 다음 파일들을 추가하세요:

```
resource/
├── mesh.obj          # 테스트할 3D 메시 파일 (필수)
├── texture.jpg       # 메시 텍스처 파일 (선택, 없으면 색상만 렌더링)
```

**참고:**
- OBJ 파일은 vertices (v), texture coordinates (vt), normals (vn), faces (f) 포맷을 지원합니다.
- 텍스처는 JPG/PNG 형식을 지원합니다.
- `resource/` 폴더는 `.gitignore`에 추가되어 Git에 업로드되지 않습니다.

## reference

- paper: Garland, M., & Heckbert, P. S. (1997). "Surface simplification using quadric error metrics." *SIGGRAPH 97*.

- model: [Sketchfab](https://skfb.ly/6nqP6), made by **Moon dong hwa**
