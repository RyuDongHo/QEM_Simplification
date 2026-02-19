#ifndef QEM_H
#define QEM_H

/**
 * QEM.h
 *
 * Quadric Error Metric (QEM) 알고리즘 구현
 *
 * 참고 논문:
 * Garland, M., & Heckbert, P. S. (1997).
 * "Surface simplification using quadric error metrics." SIGGRAPH 97.
 */

#include "Mesh.h"

/**
 * Compute collapse cost for an edge
 *
 * QEM 공식을 사용하여 edge collapse 비용 계산:
 * 1. Q_edge = Q_v1 + Q_v2 (quadric 합산)
 * 2. 최적 위치 v* = argmin(v^T · Q · v)
 * 3. Cost = v*^T · Q · v*
 *
 * @param edge 계산할 edge (cost와 optimalPosition이 업데이트됨)
 * @param vertices 메시의 모든 vertex (각 vertex는 quadric을 가짐)
 * @param faces 메시의 모든 face (unused in this function)
 */

void computeCost(Edge &edge, const std::vector<Vertex> &vertices)
{
  // Combine quadrics from both vertices
  // Q_edge = Q_v1 + Q_v2
  glm::mat4 Q = vertices[edge.v1].quadric + vertices[edge.v2].quadric;

  // Compute optimal collapse position
  // Modify last row to enforce w=1 constraint (homogeneous coordinates)
  glm::mat4 Q_bar = Q;
  Q_bar[3] = glm::vec4(0, 0, 0, 1); // [0,0,0,1] for w=1 constraint

  glm::vec4 optimalPos;
  if (glm::determinant(Q_bar) != 0)
  {
    // Solve linear system: Q_bar · v = [0,0,0,1]^T
    // Result: optimal position with w=1
    optimalPos = glm::inverse(Q_bar) * glm::vec4(0, 0, 0, 1);
  }
  else
  {
    // Singular matrix: fallback to midpoint
    // TODO: Try v1, v2, midpoint and pick minimum cost
    optimalPos = glm::vec4((vertices[edge.v1].position + vertices[edge.v2].position) * 0.5f, 1.0f);
  }

  // Store optimal position (vec4 → vec3)
  edge.optimalPosition = glm::vec3(optimalPos);

  // Compute collapse cost: error = v^T · Q · v
  // Note: Use original Q (not Q_bar) for accurate error measurement
  edge.cost = glm::dot(optimalPos, Q * optimalPos);
}

/**
 * Compute quadric matrix for a vertex
 *
 * Vertex의 quadric = 인접한 모든 face들의 fundamental quadric 합
 * Q_vertex = Σ Kp (for all adjacent faces)
 *
 * Kp = p · p^T (outer product)
 * where p = [a, b, c, d]^T = plane equation
 *
 * @param vertexIndex 계산할 vertex의 인덱스
 * @param vertices 메시의 모든 vertex
 * @param faces 메시의 모든 face
 */

void computeQuadric(int vertexIndex, std::vector<Vertex> &vertices, const std::vector<Face> &faces)
{
  // Initialize quadric to zero matrix
  vertices[vertexIndex].quadric = glm::mat4(0.0f);
  
  // Sum quadrics from all adjacent faces
  for (const Face &face : faces)
  {
    if (face.isDeleted)
      continue; // Skip deleted faces
      
    // Check if this vertex belongs to the face (by index comparison)
    if (face.v1 == vertexIndex || face.v2 == vertexIndex || face.v3 == vertexIndex)
    {
      // Get plane equation: p = [a, b, c, d]^T
      glm::vec4 p = face.planeEquation;
      
      // Compute fundamental quadric: Kp = p · p^T (outer product)
      // Result is 4x4 symmetric matrix
      glm::mat4 Kp = glm::outerProduct(p, p);
      
      // Add to vertex quadric
      vertices[vertexIndex].quadric += Kp;
    }
  }
}

/**
 * Edge collapse operation
 *
 * Edge를 collapse하여 vertex를 병합:
 * 1. 새로운 vertex 위치 = edge.optimalPosition
 * 2. 영향받는 face들 업데이트
 * 3. Degenerate face 제거 (area=0 면)
 * 4. 인접 edge들의 cost 재계산
 *
 * @param mesh 메시 데이터 (vertices, faces가 수정됨)
 * @param edge collapse할 edge
 *
 * TODO: 현재는 미완성 (placeholder)
 */
void edgeCollapse(Mesh &mesh, Edge &edge)
{
  // vertex 통합 및 삭제
  glm::vec3 newPosition = edge.optimalPosition;
  mesh.vertices[edge.v1].position = newPosition;
  mesh.vertices[edge.v2].position = newPosition;
  mesh.vertices[edge.v2].isDeleted = true; // Mark second vertex as deleted (for lazy updates)
  mesh.deletedVertices += 1;               // Increment deleted vertex count

  // edge 삭제
  edge.isDeleted = true; // Mark edge as deleted (for lazy updates)

  // 다른 edge들 업데이트: v2를 포함하는 edge들을 v1으로 remap 또는 삭제
  for (Edge &e : mesh.edges)
  {
    if (e.isDeleted)
      continue;

    // v2를 포함하는 edge를 v1으로 remap
    if (e.v1 == edge.v2)
      e.v1 = edge.v1;
    if (e.v2 == edge.v2)
      e.v2 = edge.v1;

    // Degenerate edge (v1 == v2) 제거
    if (e.v1 == e.v2)
    {
      e.isDeleted = true;
    }
  }

  // face 업데이트
  for (Face &face : mesh.faces)
  {
    if (face.isDeleted) continue; // 이미 삭제된 face는 스킵
    
    // Update faces that reference the collapsed edge's vertices
    if (face.v1 == edge.v2)
      face.v1 = edge.v1;
    if (face.v2 == edge.v2)
      face.v2 = edge.v1;
    if (face.v3 == edge.v2)
      face.v3 = edge.v1;

    // Degenerate face (중복된 vertex) 삭제
    if(face.v1 == face.v2 || face.v2 == face.v3 || face.v3 == face.v1){
      face.isDeleted = true;
    }
  }

  // new vertex의 quadric 업데이트
  computeQuadric(edge.v1, mesh.vertices, mesh.faces);

  // uv, color 업데이트
  mesh.vertices[edge.v1].texCoord = (mesh.vertices[edge.v1].texCoord + mesh.vertices[edge.v2].texCoord) * 0.5f;
  mesh.vertices[edge.v1].color = (mesh.vertices[edge.v1].color + mesh.vertices[edge.v2].color) * 0.5f;
}

#endif // QEM_H