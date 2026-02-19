#ifndef VERTEX_H
#define VERTEX_H

/**
 * Vertex.h
 *
 * 메시의 정점 (Mesh Vertex)
 * - Position: 3D 위치
 * - Normal: 정점 법선 (인접 면들의 평균)
 * - TexCoord: 텍스처 UV 좌표
 * - Color: 정점 색상 (texture 없을 때 사용)
 * - Quadric: QEM 알고리즘의 quadric error matrix (4x4 symmetric)
 * - AdjacentVertices: 인접 정점 인덱스 (토폴로지 관리용, 선택사항)
 */

#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Vertex
{
public:
  glm::vec3 position;                // 3D position
  glm::vec3 normal;                  // Vertex normal
  glm::vec2 texCoord;                // Texture UV coordinates
  glm::vec4 color;                   // Vertex color (RGBA)
  std::vector<int> adjacentVertices; // Adjacent vertex indices (optional)
  glm::mat4 quadric;                 // QEM quadric matrix Q (4x4)
  bool isDeleted;                    // Vertex deletion flag (for simplification)

  /**
   * Constructor
   *
   * @param pos 정점 위치
   * @param norm 법선 벡터
   * @param uv 텍스처 좌표
   * @param col 색상
   */
  Vertex(const glm::vec3 &pos, const glm::vec3 &norm, const glm::vec2 &uv, const glm::vec4 &col)
      : position(pos), normal(norm), texCoord(uv), color(col), quadric(glm::mat4(0.0f)), isDeleted(false) {}
};

#endif // VERTEX_H