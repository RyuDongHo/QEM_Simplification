#ifndef FACE_H
#define FACE_H

#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Face
{
public:
  int v1, v2, v3; // vertex indices
  glm::vec3 normal;
  glm::vec4 planeEquation; // [a, b, c, d] for ax + by + cz + d = 0
  bool isDeleted; // Face deletion flag (for simplification)

  void computeNormal(const glm::vec3 &pos1, const glm::vec3 &pos2, const glm::vec3 &pos3){
    // Compute normal from cross product
    glm::vec3 edge1 = pos2 - pos1;
    glm::vec3 edge2 = pos3 - pos1;
    normal = glm::normalize(glm::cross(edge1, edge2));

    // Compute d from plane equation: dot(normal, point) + d = 0
    float d = -glm::dot(normal, pos1);

    planeEquation = glm::vec4(normal.x, normal.y, normal.z, d);
  }

  // Constructor with vertex positions
  Face(int idx1, int idx2, int idx3,
       const glm::vec3 &pos1, const glm::vec3 &pos2, const glm::vec3 &pos3)
      : v1(idx1), v2(idx2), v3(idx3), isDeleted(false)
  {
    computeNormal(pos1, pos2, pos3);
  }
};

#endif // FACE_H