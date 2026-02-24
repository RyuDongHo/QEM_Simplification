
/*
 * QEM Simplification - Main Application
 *
 * Quadric Error Metric (QEM) 기반 메시 단순화 애플리케이션
 * - GLB 파일 로딩 및 렌더링 (embedded texture 지원)
 * - Trackball 카메라 컨트롤
 * - QEM 알고리즘을 통한 메시 단순화
 */

#define GLM_ENABLE_EXPERIMENTAL
#include <stdio.h>
#include <string.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader.h"
#include <vector>
#include <math.h>
#include <algorithm>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "common.h"
#include "Mesh.h"
#include <map>
#include <queue>
#include "QEM.h"

// =============================================================================
// Global Variables
// =============================================================================

// Window Configuration
const GLuint WIN_W = 1600; // Window width
const GLuint WIN_H = 900;	 // Window height
const GLuint WIN_X = 800;	 // Window X position
const GLuint WIN_Y = 450;	 // Window Y position
float aspectRatio = (float)WIN_W / (float)WIN_H;
GLFWwindow *window;

// Mesh Data
Mesh mesh;										// Main mesh data structure (vertices, edges, faces)
int simplificationLevel = 0;	// Current simplification level (for testing)
size_t activeVertexCount = 0; // Number of active (non-deleted) vertices for rendering

// OpenGL Resources
GLuint vao;				// Vertex Array Object
GLuint vbo;				// Vertex Buffer Object
GLuint textureID; // Texture ID for mesh rendering
GLuint programID; // Shader program ID

// Camera Configuration
float theta = 0.f; // Camera rotation angle (unused currently)
float fov = 45.f;	 // Field of view (adjustable with J/K keys)

// Trackball Camera Control
glm::mat4 matDrag = glm::mat4(1.f);		 // Current drag rotation matrix
glm::mat4 matUpdated = glm::mat4(1.f); // Accumulated rotation matrix
int dragMode = GL_FALSE;							 // Mouse drag state
glm::vec2 dragStart = glm::vec2(1.f);	 // Drag start position

// MVP Matrices
glm::mat4 matModel = glm::mat4(1.f); // Model matrix (object transform)
glm::mat4 matView = glm::mat4(1.f);	 // View matrix (camera transform)
glm::mat4 matProj = glm::mat4(1.f);	 // Projection matrix

// Unused legacy variable
GLfloat color[4] = {0.933f, 0.769f, 0.898f, 1.0f};

unsigned int originalVertexCount = 0; // Original vertex count (for tracking simplification progress)

// =============================================================================
// Callback Functions
// =============================================================================

/**
 * OpenGL Debug Callback
 * 디버깅 메시지를 콘솔에 출력 (OpenGL 오류 추적용)
 */
void DebugLog(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
							const GLchar *message, const GLvoid *userParam)
{
	printf("Type: %#x; Source: %#x; ID: %d; Severity: %#x\n", type, source, id, severity);
	printf("Message: %s\n", message);
	fflush(stdout);
}

// =============================================================================
// Rendering Functions
// =============================================================================

/**
 * Update VBO with current mesh data
 *
 * Mesh 데이터를 GPU VBO에 업로드
 * - Simplification 후 또는 mesh 변경 시에만 호출해야 함
 * - 매 프레임 호출하면 성능 저하 발생
 *
 * @return 렌더링할 vertex 수 (삭제된 vertex 제외)
 */

// Extract rendering data from mesh
std::vector<glm::vec4> verticesVec4; // Position data (vec3 → vec4 for homogeneous coords)
std::vector<glm::vec4> colors;			 // Vertex colors
std::vector<glm::vec2> uvs;					 // Texture coordinates
size_t updateRenderData()
{
	verticesVec4.clear();
	colors.clear();
	uvs.clear();
	
	// Pre-allocate memory for better performance
	size_t estimatedSize = mesh.faces.size() * 3;
	verticesVec4.reserve(estimatedSize);
	colors.reserve(estimatedSize);
	uvs.reserve(estimatedSize);
	
	// Render based on FACES, not vertices
	// Each face contributes 3 vertices to the rendering buffer
	for (const Face &face : mesh.faces)
	{
		if (face.isDeleted)
			continue; // Skip deleted faces
		
		// Add the 3 vertices of this face
		const Vertex &v1 = mesh.vertices[face.v1];
		const Vertex &v2 = mesh.vertices[face.v2];
		const Vertex &v3 = mesh.vertices[face.v3];
		
		// Positions
		verticesVec4.push_back(glm::vec4(v1.position, 1.0f));
		verticesVec4.push_back(glm::vec4(v2.position, 1.0f));
		verticesVec4.push_back(glm::vec4(v3.position, 1.0f));
		
		// Colors
		colors.push_back(v1.color);
		colors.push_back(v2.color);
		colors.push_back(v3.color);
		
		// UVs
		uvs.push_back(v1.texCoord);
		uvs.push_back(v2.texCoord);
		uvs.push_back(v3.texCoord);
	}

	// Calculate buffer sizes
	size_t vertexSize = verticesVec4.size() * sizeof(glm::vec4);
	size_t colorSize = colors.size() * sizeof(glm::vec4);
	size_t uvSize = uvs.size() * sizeof(glm::vec2);

	// Upload data to GPU
	// Layout: [positions | colors | uvs]
	glBufferData(GL_ARRAY_BUFFER, vertexSize + colorSize + uvSize, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertexSize, verticesVec4.data());
	glBufferSubData(GL_ARRAY_BUFFER, vertexSize, colorSize, colors.data());
	glBufferSubData(GL_ARRAY_BUFFER, vertexSize + colorSize, uvSize, uvs.data());

	// Setup vertex attributes (match shader layout locations)
	// Location 0: position (vec4)
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (void *)0);
	glEnableVertexAttribArray(0);
	// Location 1: color (vec4)
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void *)vertexSize);
	glEnableVertexAttribArray(1);
	// Location 2: texCoord (vec2)
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void *)(vertexSize + colorSize));
	glEnableVertexAttribArray(2);

	return verticesVec4.size(); // Return actual vertex count
}

/**
 * Edge Comparator for priority queue
 * Min-heap: 작은 cost가 우선순위가 높음 (먼저 collapse)
 */
struct EdgeComparator
{
	bool operator()(const Edge &a, const Edge &b) const
	{
		return a.cost > b.cost; // Min-heap based on collapse cost
	}
};

std::priority_queue<Edge, std::vector<Edge>, EdgeComparator> edgeQueue;

/**
 * Mesh Simplification using QEM
 * 
 * Lazy evaluation: edge cost는 필요할 때만 재계산
 * isDirty flag로 cost가 오래되었는지 추적
 * 
 * 한 번 호출 시 하나의 edge만 collapse
 */
void meshSimplify()
{
	// Initialize queue on first call (when empty)
	if (edgeQueue.empty())
	{
		for (int i = 0; i < mesh.edges.size(); i++)
		{
			if (mesh.edges[i].isDeleted)
				continue; // Skip deleted edges
			computeCost(mesh.edges[i], mesh.vertices);
			mesh.edges[i].isDirty = false;
			edgeQueue.push(mesh.edges[i]);
		}
	}

	int count = 0;
	// Perform ONE edge collapse per call
	while (!edgeQueue.empty())
	{
		Edge edge = edgeQueue.top();
		edgeQueue.pop();

		// Find the edge in mesh.edges array (to check isDirty and isDeleted)
		int edgeIndex = -1;
		for (int i = 0; i < mesh.edges.size(); i++)
		{
			// Check both directions since edge can be stored as (v1,v2) or (v2,v1)
			if ((mesh.edges[i].v1 == edge.v1 && mesh.edges[i].v2 == edge.v2) ||
			    (mesh.edges[i].v1 == edge.v2 && mesh.edges[i].v2 == edge.v1))
			{
				edgeIndex = i;
				break;
			}
		}

		if (edgeIndex == -1 || mesh.edges[edgeIndex].isDeleted)
		{
			// Edge already deleted, try next one
			continue;
		}

		// Check if cost is outdated (lazy evaluation)
		if (mesh.edges[edgeIndex].isDirty)
		{
			// Recompute cost and reinsert into queue
			computeCost(mesh.edges[edgeIndex], mesh.vertices);
			mesh.edges[edgeIndex].isDirty = false;
			edgeQueue.push(mesh.edges[edgeIndex]);
			continue; // Try next edge
		}

		// Perform edge collapse
		edgeCollapse(mesh, mesh.edges[edgeIndex]);

		int newVertexIndex = mesh.edges[edgeIndex].v1; // Use actual edge from mesh, not queue copy

		// Mark affected edges as dirty and reinsert into queue
		for (int i = 0; i < mesh.edges.size(); i++)
		{
			if (mesh.edges[i].isDeleted)
				continue;
			if (mesh.edges[i].v1 == newVertexIndex || mesh.edges[i].v2 == newVertexIndex)
			{
				mesh.edges[i].isDirty = true;
				// Reinsert into queue so it gets reevaluated
				edgeQueue.push(mesh.edges[i]);
			}
		}
		++count;
		if(count >= originalVertexCount / 100) break;
	}
}
/**
 * Initialize OpenGL resources
 *
 * OpenGL 초기화:
 * - Shader 로딩
 * - VAO/VBO 생성
 * - 초기 VBO 데이터 업로드
 * - Depth test 및 face culling 설정
 */
void initFunc()
{
	// Load and compile shaders
	programID = LoadShader("../../shader/vertex.glsl", "../../shader/fragment.glsl");
	if (programID == 0)
	{
		printf("Shader loading failed!\n");
		return;
	}
	originalVertexCount = mesh.vertices.size(); // Store original vertex count for tracking
	// Create and bind VAO (Vertex Array Object)
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create and bind VBO (Vertex Buffer Object)
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	// Upload initial mesh data to GPU
	printf("Uploading mesh data to GPU...\n");
	activeVertexCount = updateRenderData();
	printf("GPU upload complete. Active vertex count: %zu\n", activeVertexCount);

	// Set clear color (sky blue background)
	glClearColor(0.5f, 0.8f, 0.8f, 1.0f);
	glClearDepthf(1.0f);
	glUseProgram(programID);

	// Disable face culling for GLB compatibility (some models have flipped normals)
	glDisable(GL_CULL_FACE);
	
	printf("OpenGL initialization complete\n");
}

/**
 * Update per-frame data
 *
 * 프레임마다 업데이트:
 * - 카메라 뷰 행렬 (현재는 고정 위치)
 * - 투영 행렬 (FOV 변경 시 업데이트)
 */
void updateFunc()
{
	float elapsedTime = (float)glfwGetTime();
	theta = elapsedTime * (3.141592f / 2.f);

	// Camera setup
	matView = glm::lookAt(
			// Camera position (could enable rotation with theta)
			glm::vec3(50.f, 50.f, 50.f), // Eye position
			glm::vec3(0.f, 10.f, 0.f),							// Look-at target (origin)
			glm::vec3(0.f, 1.f, 0.f));						// Up vector

	// Projection matrix (perspective)
	matProj = glm::perspectiveRH(
			glm::radians(fov), // Field of view (adjustable with J/K keys)
			aspectRatio,			 // Aspect ratio (width/height)
			0.1f,							 // Near clipping plane
			5000.0f);						 // Far clipping plane
}

/**
 * Render the scene
 *
 * 렌더링 파이프라인:
 * 1. Clear buffers
 * 2. Set MVP matrices
 * 3. Bind texture
 * 4. Draw main viewport
 * 5. Draw mini-map viewport (top-right corner)
 */
void drawFunc()
{
	// Clear color and depth buffers
	glClearColor(0.5f, 0.8f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthRange(0.f, 1.f);

	// Get current window size
	GLint win_w, win_h;
	glfwGetWindowSize(window, &win_w, &win_h);
	aspectRatio = (float)win_w / (float)win_h;

	// Mini-map viewport configuration (top-right corner)
	const float MINIMAP_X_RATIO = 0.7f;
	const float MINIMAP_Y_RATIO = 0.05f;
	const float MINIMAP_SIZE_RATIO = 0.25f;
	GLint map_x = (GLint)(win_w * MINIMAP_X_RATIO);
	GLint map_y = (GLint)(win_h * MINIMAP_Y_RATIO);
	GLsizei map_w = (GLsizei)(win_w * MINIMAP_SIZE_RATIO);
	GLsizei map_h = (GLsizei)(win_h * MINIMAP_SIZE_RATIO);

	// Bind shader and VAO
	glBindVertexArray(vao);
	glUseProgram(programID);

	// Upload MVP matrices to shader uniforms
	GLuint loc;
	loc = glGetUniformLocation(programID, "modelMat");
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matModel));

	loc = glGetUniformLocation(programID, "viewMat");
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matView));

	loc = glGetUniformLocation(programID, "projMat");
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matProj));

	// Bind texture if available
	if (textureID != 0)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);
		loc = glGetUniformLocation(programID, "textureSampler");
		glUniform1i(loc, 0); // Texture unit 0
		loc = glGetUniformLocation(programID, "useTexture");
		glUniform1i(loc, 1); // Enable texture in shader
	}
	else
	{
		loc = glGetUniformLocation(programID, "useTexture");
		glUniform1i(loc, 0); // Use vertex colors instead
	}

	// Draw main viewport (full screen)
	glViewport(0, 0, win_w, win_h);
	glDrawArrays(GL_TRIANGLES, 0, verticesVec4.size());

	// Draw mini-map viewport (top-right corner)
	glEnable(GL_SCISSOR_TEST);
	glScissor(map_x, map_y, map_w, map_h);
	glViewport(map_x, map_y, map_w, map_h);
	glClearColor(0.5f, 0.5f, 1.f, 1.f); // Bluish background for mini-map
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, verticesVec4.size());
	glScissor(0, 0, win_w, win_h);
	glDisable(GL_SCISSOR_TEST);

	glFinish();
}

// =============================================================================
// Input Callback Functions
// =============================================================================

/**
 * Mouse cursor position callback (for trackball rotation)
 */
void cursorPosFunc(GLFWwindow *win, double xscr, double yscr)
{
	if (dragMode)
	{
		// Calculate trackball rotation from mouse drag
		glm::vec2 dragCur = glm::vec2((GLfloat)xscr, (GLfloat)yscr);
		matDrag = calcTrackball(dragStart, dragCur, (float)WIN_W, (float)WIN_H);
		// Apply rotation to model matrix
		matModel = matDrag * matUpdated;
	}
}

/**
 * Mouse button callback (start/end trackball rotation)
 */
void mouseButtonFunc(GLFWwindow *win, int button, int action, int mods)
{
	GLdouble x, y;
	switch (action)
	{
	case GLFW_PRESS:
		// Start dragging
		dragMode = GL_TRUE;
		glfwGetCursorPos(win, &x, &y);
		dragStart = glm::vec2((GLfloat)x, (GLfloat)y);
		break;

	case GLFW_RELEASE:
		// End dragging and accumulate rotation
		dragMode = GL_FALSE;
		glfwGetCursorPos(win, &x, &y);
		glm::vec2 dragCur = glm::vec2((GLfloat)x, (GLfloat)y);
		matDrag = calcTrackball(dragStart, dragCur, (float)WIN_W, (float)WIN_H);
		matModel = matDrag * matUpdated;
		matDrag = glm::mat4(1.0F); // Reset drag matrix
		matUpdated = matModel;		 // Save accumulated rotation
		break;
	}
	fflush(stdout);
}

/**
 * Keyboard callback
 *
 * Controls:
 * - ESC: Exit application
 * - J/K: Increase/decrease FOV
 * - SPACE: Trigger simplification (WIP)
 */
void keyFunc(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	switch (key)
	{
	case GLFW_KEY_ESCAPE:
		if (action == GLFW_PRESS)
		{
			printf("Exiting application\n");
			fflush(stdout);
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		break;

	case GLFW_KEY_J:
		if (action == GLFW_PRESS)
		{
			// Increase field of view (zoom out)
			fov = std::clamp(fov + 5.f, 0.f, 120.f);
			printf("FOV: %.1f\n", fov);
		}
		break;

	case GLFW_KEY_K:
		if (action == GLFW_PRESS)
		{
			// Decrease field of view (zoom in)
			fov = std::clamp(fov - 5.f, 0.f, 120.f);
			printf("FOV: %.1f\n", fov);
		}
		break;

	case GLFW_KEY_SPACE:
	{
		// Trigger mesh simplification (implementation in progress)
		simplificationLevel++;
		meshSimplify();
		activeVertexCount = updateRenderData(); // Refresh VBO after simplification
		break;
	}

	default:
		break;
	}
}
void refreshFunc(GLFWwindow *window)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glfwSwapBuffers(window);
}

// =============================================================================
// Main Function
// =============================================================================

int main(int argc, char *arvg[])
{

	// -------------------------------------------------------------------------
	// 1. Initialize GLFW and create window
	// -------------------------------------------------------------------------
	glfwInit();
	window = glfwCreateWindow(WIN_W, WIN_H, "QEM Mesh Simplification", NULL, NULL);
	glfwSetWindowPos(window, WIN_X, WIN_Y);
	glfwMakeContextCurrent(window);

	// Initialize GLEW (OpenGL extension loader)
	glewInit();

	// Register input callbacks
	glfwSetKeyCallback(window, keyFunc);
	glfwSetCursorPosCallback(window, cursorPosFunc);
	glfwSetMouseButtonCallback(window, mouseButtonFunc);

	// -------------------------------------------------------------------------
	// 2. Load GLB mesh file (with embedded texture)
	// -------------------------------------------------------------------------
	std::vector<glm::vec3> vertices; // Temporary storage for GLB data
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	int numVertices = 0;

	bool res = loadGLB("../../resource/mesh.glb", vertices, uvs, normals, &textureID);
	if (!res)
	{
		printf("Failed to load GLB file!\n");
		return -1;
	}
	numVertices = vertices.size();
	printf("Loaded %d vertices\n", numVertices);

	// -------------------------------------------------------------------------
	// 3. Build mesh data structure (Vertex, Edge, Face)
	// -------------------------------------------------------------------------
	mesh.buildMesh(numVertices, vertices, uvs, normals);
	printf("Mesh: %zu vertices, %zu faces, %zu edges\n",
				 mesh.vertices.size(), mesh.faces.size(), mesh.edges.size());

	// -------------------------------------------------------------------------
	// 3.5. Initialize Quadrics for all vertices
	// -------------------------------------------------------------------------
	printf("Initializing quadrics for %zu vertices...\n", mesh.vertices.size());
	computeAllQuadrics(mesh.vertices, mesh.faces);
	printf("Quadrics initialized for all vertices\n");

	// -------------------------------------------------------------------------
	// 4. Check texture loading status
	// -------------------------------------------------------------------------
	if (textureID == 0) {
		printf("Warning: No embedded texture in GLB file, using vertex colors\n");
	}

	// -------------------------------------------------------------------------
	// 5. Initialize OpenGL (shaders, VAO, VBO)
	// -------------------------------------------------------------------------
	// Enable OpenGL debug output for error tracking
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
	glDebugMessageCallback(DebugLog, NULL);

	initFunc();

	// -------------------------------------------------------------------------
	// 6. Main rendering loop
	// -------------------------------------------------------------------------
	while (!glfwWindowShouldClose(window))
	{
		// Update per-frame data (camera, projection)
		updateFunc();

		// Render scene
		drawFunc();

		// Check for OpenGL errors
		GLenum err = glGetError();
		if (err != GL_NO_ERROR)
		{
			printf("OpenGL error: 0x%x\n", err);
			fflush(stdout);
		}

		// Swap buffers and poll events
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// -------------------------------------------------------------------------
	// 7. Cleanup
	// -------------------------------------------------------------------------
	glfwTerminate();
	return 0;
}