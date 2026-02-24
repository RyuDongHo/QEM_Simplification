/**
 * QEM.cpp - Implementation
 *
 * Quadric Error Metric (QEM) 알고리즘 구현 (개선 버전)
 */

#include "../includes/QEM.h"

void computeCost(Edge &edge, const std::vector<Vertex> &vertices)
{
  // Combine quadrics from both vertices
  // Q_edge = Q_v1 + Q_v2
  glm::mat4 Q = vertices[edge.v1].quadric + vertices[edge.v2].quadric;

  // Compute optimal collapse position
  // Modify last row to enforce w=1 constraint (homogeneous coordinates)
  glm::mat4 Q_bar = Q;
  Q_bar[0][3] = 0;                  // 첫 번째 열의 4번째 행
  Q_bar[1][3] = 0;                  // 두 번째 열의 4번째 행
  Q_bar[2][3] = 0;                  // 세 번째 열의 4번째 행
  Q_bar[3][3] = 1;                  // 네 번째 열의 4번째 행

  glm::vec4 optimalPos;
  float minCost = std::numeric_limits<float>::max();

  // Check if Q_bar is invertible (improved numerical stability)
  float det = glm::determinant(Q_bar);
  if (std::abs(det) > QEM_EPSILON)
  {
    // Solve linear system: Q_bar · v = [0,0,0,1]^T
    // Result: optimal position with w=1
    optimalPos = glm::inverse(Q_bar) * glm::vec4(0, 0, 0, 1);
    minCost = glm::dot(optimalPos, Q * optimalPos);
  }
  else
  {
    // Singular matrix: try v1, v2, and midpoint, pick minimum cost
    glm::vec3 candidates[3] = {
        vertices[edge.v1].position,
        vertices[edge.v2].position,
        (vertices[edge.v1].position + vertices[edge.v2].position) * 0.5f};

    for (int i = 0; i < 3; i++)
    {
      glm::vec4 candidate(candidates[i], 1.0f);
      float cost = glm::dot(candidate, Q * candidate);

      if (cost < minCost)
      {
        minCost = cost;
        optimalPos = candidate;
      }
    }
  }

  // Store optimal position (vec4 → vec3)
  edge.optimalPosition = glm::vec3(optimalPos);

  // Store collapse cost
  edge.cost = minCost;
}

void computeQuadric(int vertexIndex, std::vector<Vertex> &vertices, const std::vector<Face> &faces)
{
  // Initialize quadric to zero matrix
  vertices[vertexIndex].quadric = glm::mat4(0.0f);

  // Sum quadrics from all adjacent faces
  // NOTE: This is called per-vertex and iterates all faces - O(V*F) complexity
  // For large meshes, use computeAllQuadrics() instead for O(F) complexity
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

// Optimized: Compute all quadrics in O(F) time instead of O(V*F)
void computeAllQuadrics(std::vector<Vertex> &vertices, const std::vector<Face> &faces)
{
  // Initialize all quadrics to zero
  for (Vertex &v : vertices)
  {
    v.quadric = glm::mat4(0.0f);
  }

  // Iterate faces once and accumulate to vertex quadrics
  for (const Face &face : faces)
  {
    if (face.isDeleted)
      continue;

    // Get plane equation
    glm::vec4 p = face.planeEquation;
    glm::mat4 Kp = glm::outerProduct(p, p);

    // Add to all 3 vertices of this face
    vertices[face.v1].quadric += Kp;
    vertices[face.v2].quadric += Kp;
    vertices[face.v3].quadric += Kp;
  }
}

void edgeCollapse(Mesh &mesh, Edge &edge)
{
  int v1 = edge.v1;
  int v2 = edge.v2;
  glm::vec3 newPosition = edge.optimalPosition;

  // Step 1: vertex 통합 및 삭제
  mesh.vertices[v1].position = newPosition;
  mesh.vertices[v2].position = newPosition; // v2도 같은 위치로 (cleanup 전까지)
  mesh.vertices[v2].isDeleted = true;
  mesh.deletedVertices += 1;

  // Step 2: edge 삭제 표시
  edge.isDeleted = true;

  // Step 3: 영향받는 edge들 추적 (v1과 인접한 edge들)
  std::unordered_set<int> affectedEdgeIndices;

  // Step 4: edge들 업데이트 (v2 → v1 remap)
  for (int i = 0; i < mesh.edges.size(); i++)
  {
    Edge &e = mesh.edges[i];
    if (e.isDeleted)
      continue;

    bool wasModified = false;

    // v2를 포함하는 edge를 v1으로 remap
    if (e.v1 == v2)
    {
      e.v1 = v1;
      wasModified = true;
    }
    if (e.v2 == v2)
    {
      e.v2 = v1;
      wasModified = true;
    }

    // Degenerate edge (v1 == v2) 제거
    if (e.v1 == e.v2)
    {
      e.isDeleted = true;
      continue;
    }

    // v1과 인접한 edge 추적 (cost 재계산 대상)
    if (e.v1 == v1 || e.v2 == v1)
    {
      affectedEdgeIndices.insert(i);
    }
  }

  // Step 5: face 업데이트
  for (Face &face : mesh.faces)
  {
    if (face.isDeleted)
      continue;

    // Update faces that reference v2 → v1
    if (face.v1 == v2)
      face.v1 = v1;
    if (face.v2 == v2)
      face.v2 = v1;
    if (face.v3 == v2)
      face.v3 = v1;

    // Degenerate face (중복된 vertex) 삭제
    if (face.v1 == face.v2 || face.v2 == face.v3 || face.v3 == face.v1)
    {
      face.isDeleted = true;
    }
  }

  // Step 6: v1의 quadric 재계산 (새로운 위치와 topology 반영)
  computeQuadric(v1, mesh.vertices, mesh.faces);

  // Step 7: 영향받는 모든 edge의 cost 재계산
  for (int edgeIdx : affectedEdgeIndices)
  {
    if (!mesh.edges[edgeIdx].isDeleted)
    {
      computeCost(mesh.edges[edgeIdx], mesh.vertices);
    }
  }

  // Step 8: attribute 보간 (optimal position 기반)
  // optimal position이 v1, v2 사이 어디에 있는지에 따라 가중치 계산
  glm::vec3 v1Pos = mesh.vertices[v1].position;
  glm::vec3 v2Pos = mesh.vertices[v2].position;

  // newPosition이 v1과 v2 사이에 있다고 가정하고 보간 비율 계산
  float totalDist = glm::length(v1Pos - v2Pos);
  float t = 0.5f; // default midpoint

  if (totalDist > QEM_EPSILON)
  {
    float distToV1 = glm::length(newPosition - v1Pos);
    t = distToV1 / totalDist; // 0 = v1, 1 = v2
    t = glm::clamp(t, 0.0f, 1.0f);
  }

  // Weighted interpolation
  mesh.vertices[v1].texCoord = glm::mix(mesh.vertices[v1].texCoord,
                                        mesh.vertices[v2].texCoord, t);
  mesh.vertices[v1].color = glm::mix(mesh.vertices[v1].color,
                                     mesh.vertices[v2].color, t);
}

void initializeQuadrics(Mesh &mesh)
{
  for (int i = 0; i < mesh.vertices.size(); i++)
  {
    if (!mesh.vertices[i].isDeleted)
    {
      computeQuadric(i, mesh.vertices, mesh.faces);
    }
  }
}

void initializeEdgeCosts(Mesh &mesh)
{
  for (Edge &edge : mesh.edges)
  {
    if (!edge.isDeleted)
    {
      computeCost(edge, mesh.vertices);
    }
  }
}
