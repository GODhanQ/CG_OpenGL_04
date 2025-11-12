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
	const int IDK = -2;
	const int AXIS = -1;

	const int BODY = 0;
	const int LEFT_ARM = 1;
	const int RIGHT_ARM = 2;
	const int LEFT_LEG = 3;
	const int RIGHT_LEG = 4;

	const int BOX = 50;
	const int BOX_DOOR = 51;
	const int BOX_WALL = 52;

	const int ETC = 99;
}

struct Vertex_glm {
	glm::vec3 position;
	glm::vec3 color;
};

struct Custom_OBJ {
	std::string name;
	std::vector<Vertex_glm> vertices;
	std::vector<glm::vec3> normals;
	std::vector<unsigned int> indices;
	GLuint VBO{}, VAO{}, IBO{};

};

struct OBJ_File {
	std::string file_name;
	std::vector<Custom_OBJ> objects;
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
extern GLuint PerspectiveMatrixID, ViewMatrixID;
extern float FOV, AspectRatio, NearClip, FarClip;
extern float Camera_Movement_Factor_Scale;
extern float Camera_Rotation_Sum, Camera_Rotation_Factor, Camera_Rotation_Factor_Scale;

extern GLuint FigureTypeID;

extern GLuint ModelMatrixID;
extern glm::vec3 Model_Transform, Model_Movement_Factor, Model_Scale;
extern float Model_Movement_Factor_Scale, Rotation_Speed;
extern glm::quat Model_Orientation;
extern glm::vec3 Model_Velocity;
extern bool is_Jumping;
extern const float GRAVITY;
extern const float JUMP_FORCE;
extern float Gravity_Scale;

extern GLuint BodyMatrixID;

extern float Animation_Time, Animation_Speed;
extern GLuint LeftArmMatrixID, RightArmMatrixID;
extern glm::vec3 Arm_Offset, Arm_Rotation_Angle;
extern float Arm_Rotation_Speed, Arm_Rotation_Max_Angle;

extern GLuint LeftLegMatrixID, RightLegMatrixID;
extern glm::vec3 Leg_Offset, Leg_Rotation_Angle;
extern float Leg_Rotation_Speed, Leg_Rotation_Max_Angle;

extern bool LookAtRobot;

extern bool OpenDoor;
extern float Door_Open_Progress, doorAnimationSpeed, doorSlideHeight;
extern GLuint DoorMatrixID;

GLvoid drawScene();
GLvoid Reshape(int w, int h);

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

void Type_distinction(const std::string& name, GLuint& FigureTypeID);


AABB CalculateAABB(const std::vector<Vertex_glm>& vertices);
AABB TransformAABB(const AABB& aabb, const glm::mat4& transform);
bool CheckCollision(const AABB& a, const AABB& b);
bool IsAABBInside(const AABB& inner, const AABB& outer);
bool CheckAABBCollision(const AABB& a, const AABB& b);