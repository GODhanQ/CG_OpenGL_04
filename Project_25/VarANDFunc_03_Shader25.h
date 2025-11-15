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
	const int ORBIT = -3;
	const int IDK = -2;
	const int AXIS = -1;

	const int LIGHT = 0;
	const int CUBE = 1;
	const int PYRAMID = 2;

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
	glm::vec3 origin{ 0.0f, 0.0f, 0.0f };
	GLuint VBO{}, VAO{}, IBO{};

};

struct OBJ_File {
	std::string file_name;
	std::vector<Custom_OBJ> objects;
};

struct Light {
	glm::vec3 init_position;
	Vertex_glm light_vertex;
	glm::vec3 light_color;
	glm::vec3 strength;
	GLuint VAO{}, VBO{}, IBO{};
	unsigned int indices{ 0 };
	GLuint LightPosID{}, LightColorID{}, LightStrengthID{};

	Light() {
		init_position = glm::vec3(0.0f, 0.0f, 0.0f);
		light_vertex.position = init_position;
		light_vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
		light_color = glm::vec3(1.0f, 1.0f, 1.0f);
		strength = glm::vec3(1.0f, 1.0f, 1.0f);
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

extern GLuint FigureTypeID;
extern bool Draw_Cube, Light_On;

extern GLuint ModelMatrixID;
extern glm::vec3 Model_Transform, Model_Scale;
extern int Rotation_Mode, Revolution_Mode;

extern GLuint CubeMatrixID, PyramidMatrixID;
extern float Cube_Rotation_Angle, Pyramid_Rotation_Angle;
extern float Cube_Rotation_Factor, Pyramid_Rotation_Factor;

extern float Light_Revolution_Angle, Light_Revolution_Factor;
extern glm::vec3 Light_Trasform;

GLvoid drawScene();
GLvoid Reshape(int w, int h);
void DrawModels();

void KeyBoard(unsigned char key, int x, int y);
void KeyBoardUp(unsigned char key, int x, int y);
void SpecialKeyBoard(int key, int x, int y);
void SpecialKeyBoardUp(int key, int x, int y);
void MouseClick(int button, int state, int x, int y);
std::pair<float, float> ConvertScreenToOpenGL(int screen_x, int screen_y);

void INIT_BUFFER();

char* filetobuf(const char* file);
GLuint make_shaderProgram(const char* vertPath, const char* fragPath);
bool ReadObj(const std::string& path, OBJ_File& outFile);

void MakeStaticMatrix();
void MakeDynamicMatrix();

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
