#ifndef COMMON_H
#define COMMON_H

#include <GL/glew.h>
#include <string>

// 셰이더 파일 읽기
std::string readShaderFile(const char* filePath);

// 셰이더 컴파일
GLuint compileShader(GLenum type, const char* source);

// 셰이더 프로그램 생성
GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath);

#endif // COMMON_H
