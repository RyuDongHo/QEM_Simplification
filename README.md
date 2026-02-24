# QEM Simplification

## description

Quadric Error Metric(QEM) based mesh simplification system

## test

![QEM Simplification Demo](test.gif)

## System settings

- **Language**: C++17
- **Build system**: CMake

## Library

### Render
- **GLEW 2.3.1** - OpenGL Extension Wrangler Library
  - source: https://glew.sourceforge.net/
  - license: Modified BSD License, MIT License
  
- **GLFW 3.4** - OpenGL Framework
  - source: https://www.glfw.org/
  - license: zlib/libpng License
  
- **GLM** - OpenGL Mathematics
  - source: https://github.com/g-truc/glm
  - license: MIT License

### Utils
- **TinyGLTF** - Header-only C++ glTF 2.0 library
  - source: https://github.com/syoyo/tinygltf
  - license: MIT License
  
- **nlohmann/json 3.11.3** - JSON for Modern C++
  - source: https://github.com/nlohmann/json
  - license: MIT License
  
- **stb_image & stb_image_write** - Image loading/writing library
  - source: https://github.com/nothings/stb
  - license: MIT License / Public Domain

## How to build

### Windows (Visual Studio)

```bash
mkdir build
cd build

cmake ..

cmake --build . --config Release
```

### excution

```bash
cd build/Release
./QEM_Simplification.exe
```

## How to use

- **J key**: Decrease FOV (zoom in)
- **K key**: Increase FOV (zoom out)
- **Spacebar**: simplificate mesh

## How to add a mesh

make "resource" directory at root.

mesh file name should be "mesh.glb". Only glb files be supported.
```
resource/
├── mesh.glb
```

## reference & source
- paper: Garland, M., & Heckbert, P. S. (1997). "Surface simplification using quadric error metrics." *SIGGRAPH 97*.
- 3d model which tested: Stanford Bunny [by hackmans][https://skfb.ly/otyHx] (mesh file is not included in this project. just used for recording test video.)
