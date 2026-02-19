#include "common.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

// 셰이더 파일 읽기
std::string readShaderFile(const char* filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filePath << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// 셰이더 컴파일
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed:\n" << infoLog << std::endl;
        return 0;
    }
    return shader;
}

// 셰이더 프로그램 생성
GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCode = readShaderFile(vertexPath);
    std::string fragmentCode = readShaderFile(fragmentPath);

    if (vertexCode.empty() || fragmentCode.empty()) {
        return 0;
    }

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexCode.c_str());
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentCode.c_str());

    if (!vertexShader || !fragmentShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Program linking failed:\n" << infoLog << std::endl;
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// 마우스 위치에 따른 단위 벡터 계산
glm::vec3 calcUnitVecByMousePosition(const glm::vec2& raw, float winW, float winH) {
	glm::vec2 scr;
	scr.x = std::clamp(raw.x, 0.0F, winW);
	scr.y = std::clamp(raw.y, 0.0F, winH);
	const GLfloat radius = sqrt(winW * winW + winH * winH) / 2.f;
	glm::vec3 v;

	v.x = -1 * (scr.x - winW / 2.0F) / radius;
	v.y = (scr.y - winH / 2.0F) / radius;
	v.z = sqrtf(1.0F - v.x * v.x - v.y * v.y);
	return v;
}

// 트랙볼 회전 계산
glm::mat4 calcTrackball(const glm::vec2& start, const glm::vec2& cur, float winW, float winH) {
	glm::vec3 org = calcUnitVecByMousePosition(start, winW, winH);
	glm::vec3 dst = calcUnitVecByMousePosition(cur, winW, winH);
	glm::quat q = glm::rotation(dst, org);

	float angle = glm::angle(q);
	glm::vec3 axis = glm::axis(q);

	float sensitivity = 2.f;
	angle *= sensitivity;

	q = glm::angleAxis(angle, axis);
	glm::mat4 m = glm::toMat4(q);
	return m;
}

// OBJ Loader
bool loadOBJ(
	const char * path, 
	std::vector<glm::vec3> & out_vertices, 
	std::vector<glm::vec2> & out_uvs,
	std::vector<glm::vec3> & out_normals
){
	printf("Loading OBJ file %s...\n", path);

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> temp_vertices; 
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;


	FILE * file = fopen(path, "r");
	if( file == NULL ){
		printf("Impossible to open the file ! Are you in the right path ? See Tutorial 1 for details\n");
		getchar();
		return false;
	}

	while( 1 ){

		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break; // EOF = End Of File. Quit the loop.

		// else : parse lineHeader
		
		if ( strcmp( lineHeader, "v" ) == 0 ){
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z );
			temp_vertices.push_back(vertex);
		}else if ( strcmp( lineHeader, "vt" ) == 0 ){
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y );
			uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
			temp_uvs.push_back(uv);
		}else if ( strcmp( lineHeader, "vn" ) == 0 ){
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
			temp_normals.push_back(normal);
		}else if ( strcmp( lineHeader, "f" ) == 0 ){
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3] = {0}, uvIndex[3] = {0}, normalIndex[3] = {0};
			
			// Read the rest of the line
			char line[256];
			fgets(line, 256, file);
			
			// Try v/vt/vn format (vertex/texture/normal)
			int matches = sscanf(line, "%d/%d/%d %d/%d/%d %d/%d/%d\n", 
			                     &vertexIndex[0], &uvIndex[0], &normalIndex[0], 
			                     &vertexIndex[1], &uvIndex[1], &normalIndex[1], 
			                     &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			
			if (matches != 9){
				// Try v//vn format (vertex//normal, no texture)
				matches = sscanf(line, "%d//%d %d//%d %d//%d\n", 
				                 &vertexIndex[0], &normalIndex[0], 
				                 &vertexIndex[1], &normalIndex[1], 
				                 &vertexIndex[2], &normalIndex[2]);
				
				if (matches == 6) {
					// Ensure we have at least one default UV
					if (temp_uvs.empty()) {
						temp_uvs.push_back(glm::vec2(0.0f, 0.0f));
					}
					uvIndex[0] = uvIndex[1] = uvIndex[2] = 1;
				} else {
					// Try v v v format (vertex only, no texture/normal)
					matches = sscanf(line, "%d %d %d\n", 
					                 &vertexIndex[0], &vertexIndex[1], &vertexIndex[2]);
					
					if (matches == 3) {
						// Use default UV and normal
						if (temp_uvs.empty()) {
							temp_uvs.push_back(glm::vec2(0.0f, 0.0f));
						}
						if (temp_normals.empty()) {
							temp_normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
						}
						uvIndex[0] = uvIndex[1] = uvIndex[2] = 1;
						normalIndex[0] = normalIndex[1] = normalIndex[2] = 1;
					} else {
						printf("File can't be read by our simple parser :-( Try exporting with other options\n");
						printf("Unsupported face format: f %s", line);
						fclose(file);
						return false;
					}
				}
			}
			
			// Validate indices
			if (vertexIndex[0] == 0 || vertexIndex[1] == 0 || vertexIndex[2] == 0 ||
			    normalIndex[0] == 0 || normalIndex[1] == 0 || normalIndex[2] == 0) {
				printf("Invalid face indices in line: f %s", line);
				fclose(file);
				return false;
			}
			
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			uvIndices    .push_back(uvIndex[0]);
			uvIndices    .push_back(uvIndex[1]);
			uvIndices    .push_back(uvIndex[2]);
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);
		}else{
			// Probably a comment, eat up the rest of the line
			char stupidBuffer[1000];
			fgets(stupidBuffer, 1000, file);
		}

	}

	// For each vertex of each triangle
	for( unsigned int i=0; i<vertexIndices.size(); i++ ){

		// Get the indices of its attributes
		unsigned int vertexIndex = vertexIndices[i];
		unsigned int uvIndex = uvIndices[i];
		unsigned int normalIndex = normalIndices[i];
		
		// Check bounds
		if (vertexIndex < 1 || vertexIndex > temp_vertices.size() ||
		    uvIndex < 1 || uvIndex > temp_uvs.size() ||
		    normalIndex < 1 || normalIndex > temp_normals.size()) {
			printf("Index out of bounds: v=%d(max:%zu), vt=%d(max:%zu), vn=%d(max:%zu)\n",
			       vertexIndex, temp_vertices.size(), 
			       uvIndex, temp_uvs.size(), 
			       normalIndex, temp_normals.size());
			fclose(file);
			return false;
		}
		
		// Get the attributes thanks to the index
		glm::vec3 vertex = temp_vertices[ vertexIndex-1 ];
		glm::vec2 uv = temp_uvs[ uvIndex-1 ];
		glm::vec3 normal = temp_normals[ normalIndex-1 ];
		
		// Put the attributes in buffers
		out_vertices.push_back(vertex);
		out_uvs     .push_back(uv);
		out_normals .push_back(normal);
	
	}
	fclose(file);
	return true;
}

// Texture Loader
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLuint loadTexture(const char* imagepath) {
	printf("Loading texture %s...\n", imagepath);
	
	int width, height, channels;
	unsigned char* data = stbi_load(imagepath, &width, &height, &channels, 0);
	
	if (!data) {
		printf("Failed to load texture: %s\n", imagepath);
		return 0;
	}
	
	printf("Texture loaded: %dx%d, %d channels\n", width, height, channels);
	
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	
	// Set texture format based on channels
	GLenum format = GL_RGB;
	if (channels == 1)
		format = GL_RED;
	else if (channels == 3)
		format = GL_RGB;
	else if (channels == 4)
		format = GL_RGBA;
	
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	
	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	stbi_image_free(data);
	
	return textureID;
}
