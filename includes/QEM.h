#ifndef QEM_H
#define QEM_H

/**
 * QEM.h - Fixed Version
 *
 * Quadric Error Metric (QEM) 알고리즘 구현 (개선 버전)
 *
 * 참고 논문:
 * Garland, M., & Heckbert, P. S. (1997).
 * "Surface simplification using quadric error metrics." SIGGRAPH 97.
 *
 * 주요 개선 사항:
 * 1. 인접 edge cost 재계산 완전 구현
 * 2. Singular matrix 처리 개선 (3-way candidate test)
 * 3. 수치 안정성 개선 (epsilon 비교)
 * 4. Optimal position 기반 attribute 보간
 * 5. 중복 계산 방지
 */

#include "Mesh.h"
#include <unordered_set>
#include <limits>
#include <cmath>

// 수치 안정성을 위한 epsilon
const float QEM_EPSILON = 1e-10f;

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
 */
void computeCost(Edge &edge, const std::vector<Vertex> &vertices);

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
void computeQuadric(int vertexIndex, std::vector<Vertex> &vertices, const std::vector<Face> &faces);

/**
 * Compute all vertex quadrics efficiently (O(F) instead of O(V*F))
 * 
 * 모든 vertex의 quadric을 face를 한 번만 순회하여 계산
 * 대용량 메시에 최적화된 버전
 * 
 * @param vertices 메시의 모든 vertex
 * @param faces 메시의 모든 face
 */
void computeAllQuadrics(std::vector<Vertex> &vertices, const std::vector<Face> &faces);

/**
 * Edge collapse operation (완전 수정 버전)
 *
 * Edge를 collapse하여 vertex를 병합:
 * 1. 새로운 vertex 위치 = edge.optimalPosition
 * 2. 영향받는 face들 업데이트
 * 3. Degenerate face 제거 (area=0 면)
 * 4. v1의 quadric 재계산
 * 5. v1과 인접한 모든 edge의 cost 재계산
 *
 * @param mesh 메시 데이터 (vertices, faces, edges가 수정됨)
 * @param edge collapse할 edge
 */
void edgeCollapse(Mesh &mesh, Edge &edge);

/**
 * Initialize all vertex quadrics
 * 
 * 메시 simplification 시작 전 모든 vertex의 초기 quadric 계산
 * 
 * @param mesh 메시 데이터
 */
void initializeQuadrics(Mesh &mesh);

/**
 * Initialize all edge costs
 * 
 * 메시 simplification 시작 전 모든 edge의 초기 cost 계산
 * 
 * @param mesh 메시 데이터
 */
void initializeEdgeCosts(Mesh &mesh);

#endif // QEM_H