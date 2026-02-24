#ifndef MESH_H
#define MESH_H

/**
 * Mesh.h
 * 
 * Mesh 데이터 구조
 * - Vertices: 메시의 모든 정점 (position, normal, UV, quadric)
 * - Edges: 메시의 모든 간선 (중복 제거)
 * - Faces: 메시의 모든 삼각형 (plane equation 포함)
 */

#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Vertex.h"
#include "Edge.h"
#include "Face.h"
#include <unordered_map>
#include <map>

class Mesh
{
public:
  std::vector<Vertex> vertices;  // 메시의 모든 정점
  std::vector<Edge> edges;       // 메시의 모든 간선 (unique)
  std::vector<Face> faces;       // 메시의 모든 면 (triangles)
  int deletedVertices = 0;         // 삭제된 정점 수 (simplification 진행 상황 추적용)

  /**
   * Build mesh from GLB data
   * 
   * GLB 데이터로부터 Mesh 구조 생성:
   * 1. Vertices 생성 (position, normal, UV)
   * 2. Vertex welding (중복 정점 제거)
   * 3. Faces 생성 (triangulated, plane equation 계산)
   * 4. Edges 추출 (Face로부터, 중복 제거)
   * 
   * @param numVertices 입력 vertex 개수 (unrolled triangles)
   * @param vertices 정점 위치 배열
   * @param uvs 텍스처 좌표 배열
   * @param normals 법선 벡터 배열
   */

  void buildMesh(int numVertices, const std::vector<glm::vec3> &vertices,
                 const std::vector<glm::vec2> &uvs, const std::vector<glm::vec3> &normals)
  {
    // -----------------------------------------------------------------------
    // Step 1: Vertex welding with spatial hashing (O(N))
    // -----------------------------------------------------------------------
    printf("Building mesh with %d input vertices...\n", numVertices);
    printf("Performing vertex welding...\n");
    
    const float GRID_SIZE = 0.001f;
    const float EPSILON = 0.0001f;
    std::map<std::tuple<int,int,int>, std::vector<int>> spatialHash;
    std::vector<int> vertexMapping(numVertices); // unrolled index -> unique index
    
    for (int i = 0; i < numVertices; ++i)
    {
      if (i % 10000 == 0 && i > 0) {
        printf("  Processing vertex %d/%d...\n", i, numVertices);
      }
      
      const glm::vec3& pos = vertices[i];
      int gx = (int)std::floor(pos.x / GRID_SIZE);
      int gy = (int)std::floor(pos.y / GRID_SIZE);
      int gz = (int)std::floor(pos.z / GRID_SIZE);
      auto key = std::make_tuple(gx, gy, gz);
      
      bool found = false;
      auto it = spatialHash.find(key);
      if (it != spatialHash.end()) {
        for (int existingIdx : it->second) {
          const glm::vec3& existingPos = this->vertices[existingIdx].position;
          if (glm::distance(pos, existingPos) < EPSILON) {
            vertexMapping[i] = existingIdx;
            found = true;
            break;
          }
        }
      }
      
      if (!found) {
        int newIdx = this->vertices.size();
        this->vertices.push_back(Vertex(pos, normals[i], uvs[i], glm::vec4(1.0f)));
        spatialHash[key].push_back(newIdx);
        vertexMapping[i] = newIdx;
      }
    }
    
    printf("Vertex welding complete: %d -> %zu unique vertices\n", numVertices, this->vertices.size());

    // -----------------------------------------------------------------------
    // Step 2: Build faces with remapped indices
    // -----------------------------------------------------------------------
    for (int i = 0; i < numVertices; i += 3)
    {
      if (i + 2 < numVertices)
      {
        int v1 = vertexMapping[i];
        int v2 = vertexMapping[i + 1];
        int v3 = vertexMapping[i + 2];
        
        // Skip degenerate faces
        if (v1 == v2 || v2 == v3 || v3 == v1) continue;
        
        Face face(v1, v2, v3,
                  this->vertices[v1].position,
                  this->vertices[v2].position,
                  this->vertices[v3].position);
        faces.push_back(face);
      }
    }

    // -----------------------------------------------------------------------
    // Step 3: Extract unique edges from faces
    // -----------------------------------------------------------------------
    // Use map to filter duplicate edges
    // Edge (v1, v2) === Edge (v2, v1), so normalize with min/max
    std::map<std::pair<int, int>, bool> edgeSet;
    
    for (const auto &face : faces)
    {
      // Each face has 3 edges: (v1,v2), (v2,v3), (v3,v1)
      auto e1 = std::make_pair(std::min(face.v1, face.v2), std::max(face.v1, face.v2));
      auto e2 = std::make_pair(std::min(face.v2, face.v3), std::max(face.v2, face.v3));
      auto e3 = std::make_pair(std::min(face.v3, face.v1), std::max(face.v3, face.v1));

      // Add edge only if not already in set
      if (!edgeSet[e1])
      {
        edges.push_back(Edge(e1.first, e1.second));
        edgeSet[e1] = true;
      }
      if (!edgeSet[e2])
      {
        edges.push_back(Edge(e2.first, e2.second));
        edgeSet[e2] = true;
      }
      if (!edgeSet[e3])
      {
        edges.push_back(Edge(e3.first, e3.second));
        edgeSet[e3] = true;
      }
    }
  }
};

#endif // MESH_H