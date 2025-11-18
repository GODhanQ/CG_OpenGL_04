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
	const int NONE = -4;
	const int ORBIT = -3;
	const int LIGHT = -2;
	const int AXIS = -1;

	const int FLOOR = 0;
	const int TANK = 1;
	const int WALL = 2;
	const int ROBOT = 3;

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

extern float WallMaxHeight;
extern bool ScaleWallHeight;
struct Maze {
	int width, height;
	std::vector<std::vector<int>> grid;
	std::vector<std::vector<float>> wallHeights;

	int startX = 1, startY = 1;
	int endX = 1, endY = 1;

	int enterPointX = 1, enterPointY = 0;
	int exitPointX = 1, exitPointY = 2;

	Maze(int M, int N) {
		if (M <= 0) M = 1;
		if (N <= 0) N = 1;
		width = 2 * M + 1;
		height = 2 * N + 1;
		grid.assign(height, std::vector<int>(width, 1));
		wallHeights.assign(height, std::vector<float>(width, 1));
	}
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

extern GLuint FloorMatrixID, TankMatrixID;
extern float Floor_Rotation_Angle, Tank_Rotation_Angle;
extern float Floor_Rotation_Factor, Tank_Rotation_Factor;

extern GLuint WallMatrixID;
extern float animation_speed_factor;

extern float Light_Revolution_Angle, Light_Revolution_Factor;
extern glm::vec3 Light_Trasform;
extern std::vector<glm::vec3> light_color_template;
extern int light_color_template_index;
extern GLuint ShininessID;

extern int Camera_Rotation_Mode;
extern glm::vec3 Camera_Rotation_Angle, Camera_Rotation_Factor;
extern bool Perspective_On;

extern GLuint RobotMatrixID;
extern bool RobotInWorld, RobotThirdPersonView;
extern glm::vec3 Robot_Position;	// Robot의 월드 좌표
extern glm::vec3 Robot_Scale;		// Robot의 스케일
extern float Robot_Rotation_Y;		// Robot의 Y축 회전 각도
extern glm::vec3 Robot_Direction;	// Robot의 진행 방향 벡터
extern float Robot_Speed;			// Robot의 이동 속도
extern float Robot_Walk_Speed, Robot_Run_Speed;

extern bool MouseLookMode;
extern float Camera_Pitch;
extern int LastMouseX, LastMouseY;
extern bool FirstMouse;
extern float MouseSensitivity;

extern bool WaitingForMazeInput;

GLvoid drawScene();
GLvoid Reshape(int w, int h);
void DrawModels(float deltaTime);

void KeyBoard(unsigned char key, int x, int y);
void KeyBoardUp(unsigned char key, int x, int y);
void SpecialKeyBoard(int key, int x, int y);
void SpecialKeyBoardUp(int key, int x, int y);
void MouseClick(int button, int state, int x, int y);
void MouseMotion(int x, int y);
std::pair<float, float> ConvertScreenToOpenGL(int screen_x, int screen_y);

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
void MakeLightSources();

void Type_distinction(const std::string& name, GLuint& FigureTypeID);

AABB CalculateAABB(const std::vector<Vertex_glm>& vertices);
AABB TransformAABB(const AABB& aabb, const glm::mat4& transform);
bool CheckCollision(const AABB& a, const AABB& b);
bool IsAABBInside(const AABB& inner, const AABB& outer);
bool CheckAABBCollision(const AABB& a, const AABB& b);
AABB CalculateRobotAABB();

bool CheckRobotWallCollision(const glm::vec3& newPosition);

Maze MakeMaze(int N, int M);
void MakeRobotAtMazeEntrance();

void RenderText(const char* text, float x, float y, void* font = GLUT_BITMAP_HELVETICA_12);
void DisplayParameters(float deltaTime);
