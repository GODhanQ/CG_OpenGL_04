#pragma once
#include <GL/glew.h>
#include <gl/freeglut.h>
#include <gl/freeglut_ext.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gl/GLU.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <deque>
#include <tuple>
#include <random>
#include <chrono>
#include <algorithm>
#include <map>
#include <string>
#define NOMINMAX
#include <windows.h>

extern bool keyStates[256];
extern std::map<int, bool> specialKeyStates;

namespace Figure_Type {
	const int LIGHT = -2;
	const int AXIS = -1;

	// Basic Figures
	const int TEMP = 0;

	const int ETC = 99;
}

struct Vertex_glm {
	glm::vec3 position;
	glm::vec3 color;
	glm::vec3 normal;
};

struct Custom_OBJ {
	std::string name;
	std::vector<Vertex_glm> vertices;
	std::vector<unsigned int> indices;
	float shininess{ 32.0f };
	glm::vec3 origin{ 0.0f, 0.0f, 0.0f };
	GLuint VBO{}, VAO{}, IBO{};

};

struct OBJ_File {
	std::string file_name;
	std::vector<Custom_OBJ> objects;
};

struct Light {
	glm::vec3 light_color = glm::vec3(1.0f, 1.0f, 1.0f);
	Vertex_glm light_vertex;
	glm::vec3 init_position;

	// 감쇠 계수
	float constant = 1.0f;
	float linear = 0.09f;
	float quadratic = 0.032f;

	// 빛의 세기 추가
	float intensity;

	GLuint VAO = 0, VBO = 0, IBO = 0;

	Light(glm::vec3 pos = glm::vec3(0.0, 1.0, 0.0),
		float inten = 1.0f) : init_position(pos), intensity(inten) {
		light_vertex.position = pos;
		light_vertex.color = light_color;
		light_vertex.normal = glm::vec3(0.0, 1.0, 0.0);
	}
};

struct AABB {
	glm::vec3 min;
	glm::vec3 max;
};

extern GLint Window_width, Window_height;

extern GLuint shaderProgramID, vertexShader, fragmentShader;

extern GLuint VBO_axis, VAO_axis, IBO_axis;
extern std::vector<Vertex_glm> Axis_Vertex;
extern std::vector<unsigned int> Axis_Index;

extern glm::vec3 EYE, AT, UP, Camera_Transform, Camera_Transform_Factor;
extern GLuint PerspectiveMatrixID, ViewMatrixID, ViewPosID;
extern float FOV, AspectRatio, NearClip, FarClip;

extern GLuint ShininessID;
extern bool Light_On;

extern GLuint FigureTypeID;
extern GLuint ModelMatrixID;

GLvoid drawScene();
GLvoid Reshape(int w, int h);
void DrawModels(float deltaTime);

void KeyBoard(unsigned char key, int x, int y);
void KeyBoardUp(unsigned char key, int x, int y);
void SpecialKeyBoard(int key, int x, int y);
void SpecialKeyBoardUp(int key, int x, int y);
void MouseClick(int button, int state, int x, int y);

void INIT_BUFFER();

char* filetobuf(const char* file);
GLuint make_shaderProgram(const char* vertPath, const char* fragPath);
bool ReadObj(const std::string& path, OBJ_File& outFile);

void MakeStaticMatrix();
void MakeDynamicMatrix(float deltaTime);

void GetUniformLocations();
void UpdateUniformMatrices();
void ComposeOBJColor();
void ComposeOribit();

void Type_distinction(const std::string& name, GLuint& FigureTypeID);

AABB CalculateAABB(const std::vector<Vertex_glm>& vertices);
AABB TransformAABB(const AABB& aabb, const glm::mat4& transform);
bool CheckCollision(const AABB& a, const AABB& b);
bool IsAABBInside(const AABB& inner, const AABB& outer);
bool CheckAABBCollision(const AABB& a, const AABB& b);
