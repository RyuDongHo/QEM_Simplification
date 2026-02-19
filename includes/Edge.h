#ifndef EDGE_H
#define EDGE_H

/**
 * Edge.h
 * 
 * 메시의 간선 (Edge between two vertices)
 * - QEM 알고리즘에서 edge collapse의 대상
 * - Cost: Collapse 시 발생하는 quadric error
 * - Optimal position: Collapse 후 새로운 vertex의 최적 위치
 */

#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Edge
{
public:
  int v1;                      // First vertex index
  int v2;                      // Second vertex index
  float cost;                  // QEM collapse cost (quadric error)
  glm::vec3 optimalPosition;   // Optimal position after collapse
  bool isBoundary;             // 경계 간선 여부 (boundary edge flag)
  bool isDirty;                // Collapse 후 메시가 변경되었는지 여부 (for lazy updates)
  bool isDeleted;              // Edge deletion flag (for simplification)
  
  /**
   * Constructor
   * 
   * @param vertex1, vertex2 양 끝점의 vertex 인덱스
   */
  Edge(int vertex1, int vertex2)
      : v1(vertex1), v2(vertex2), isDeleted(false) {
        isBoundary = false;  // Default: not boundary
        isDirty = false;     // Default: not dirty
        cost = 0.f;          // Cost will be computed later
      }
};

#endif // EDGE_H