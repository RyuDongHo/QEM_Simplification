#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "common.h"
#include <iostream>
#include <cmath>

// GLM 없이 간단한 행렬 연산
const float PI = 3.14159265359f;

// 4x4 단위 행렬 생성
void createIdentityMatrix(float* matrix) {
    for (int i = 0; i < 16; i++) {
        matrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
}

// 원근 투영 행렬 생성
void createPerspectiveMatrix(float* matrix, float fov, float aspect, float near, float far) {
    float f = 1.0f / tan(fov * PI / 360.0f);
    createIdentityMatrix(matrix);
    matrix[0] = f / aspect;
    matrix[5] = f;
    matrix[10] = (far + near) / (near - far);
    matrix[11] = -1.0f;
    matrix[14] = (2.0f * far * near) / (near - far);
    matrix[15] = 0.0f;
}

// 뷰 행렬 생성 (간단한 Z축 이동)
void createViewMatrix(float* matrix, float z) {
    createIdentityMatrix(matrix);
    matrix[14] = z;
}

// 회전 행렬 생성
void createRotationMatrix(float* matrix, float angleX, float angleY, float angleZ) {
    float cosX = cos(angleX), sinX = sin(angleX);
    float cosY = cos(angleY), sinY = sin(angleY);
    float cosZ = cos(angleZ), sinZ = sin(angleZ);

    createIdentityMatrix(matrix);
    
    // Y축 회전 (좌우)
    matrix[0] = cosY;
    matrix[2] = sinY;
    matrix[8] = -sinY;
    matrix[10] = cosY;
    
    // X축 회전 추가
    float temp[16];
    for(int i = 0; i < 16; i++) temp[i] = matrix[i];
    
    matrix[5] = cosX;
    matrix[6] = -sinX;
    matrix[9] = sinX;
    matrix[10] = cosX * temp[10];
}

// 키보드 입력 처리
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

int main() {
    // GLFW 초기화
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // OpenGL 버전 설정
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 윈도우 생성
    GLFWwindow* window = glfwCreateWindow(800, 600, "QEM Simplification - Cube", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // GLEW 초기화
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // 뷰포트 설정
    glViewport(0, 0, 800, 600);
    glEnable(GL_DEPTH_TEST);

    // 셰이더 프로그램 생성
    GLuint shaderProgram = createShaderProgram("../shader/vertex.glsl", "../shader/fragment.glsl");
    if (!shaderProgram) {
        std::cerr << "Failed to create shader program" << std::endl;
        return -1;
    }

    // 큐브 버텍스 데이터 (위치 + 색상)
    float vertices[] = {
        // 위치              // 색상
        // 앞면
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
        // 뒷면
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f
    };

    unsigned int indices[] = {
        // 앞면
        0, 1, 2,  2, 3, 0,
        // 뒷면
        5, 4, 7,  7, 6, 5,
        // 왼쪽면
        4, 0, 3,  3, 7, 4,
        // 오른쪽면
        1, 5, 6,  6, 2, 1,
        // 위쪽면
        3, 2, 6,  6, 7, 3,
        // 아래쪽면
        4, 5, 1,  1, 0, 4
    };

    // VAO, VBO, EBO 생성
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // 위치 속성
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 색상 속성
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // 행렬 준비
    float model[16], view[16], projection[16];
    createViewMatrix(view, -3.0f);
    createPerspectiveMatrix(projection, 45.0f, 800.0f / 600.0f, 0.1f, 100.0f);

    // 렌더링 루프
    float rotation = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        // 배경 지우기
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 셰이더 사용
        glUseProgram(shaderProgram);

        // 회전 업데이트
        rotation += 0.01f;
        createRotationMatrix(model, rotation * 0.5f, rotation, rotation * 0.3f);

        // 유니폼 설정
        GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
        GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);

        // 큐브 그리기
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // 버퍼 교체 및 이벤트 처리
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 정리
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}
