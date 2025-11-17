#include "VarANDFunc_03_Shader27.h"

GLint Window_width{ 1200 }, Window_height{ 675 };

GLuint shaderProgramID{}, vertexShader{}, fragmentShader{};

GLuint VBO_axis{}, VAO_axis{}, IBO_axis{};

bool keyStates[256] = { false, };
std::map<int, bool> specialKeyStates;


glm::vec3 EYE(0.0f, 30.0f, 50.0f), AT(0.0f, 5.0f, 0.0f), UP(0.0f, 1.0f, 0.0f);
glm::vec3 Camera_Transform(0.0f, 0.0f, 0.0f), Camera_Transform_Factor(0.0f, 0.0f, 0.0f);
GLuint PerspectiveMatrixID{}, ViewMatrixID, ViewPosID{};
float FOV{ 45.0f }, AspectRatio{ (float)Window_width / (float)Window_height }, NearClip{ 0.1f }, FarClip{ 150.0f };

GLuint FigureTypeID{};
bool Draw_Cube{ true }, Light_On{ true };

GLuint ModelMatrixID{};
glm::vec3 Model_Transform(0.0, 0.0, 0.0), Model_Scale(1.0, 1.0, 1.0);
// 0 : stop, 1 : +Y axis, 2 : -Y axis
int Rotation_Mode{};
// 0 : stop, 1 : +Y axis, 2 : -Y axis
int Revolution_Mode{};

GLuint FloorMatrixID{}, TankMatrixID{};
float Floor_Rotation_Angle{}, Tank_Rotation_Angle{};
float Floor_Rotation_Factor{ 50.0f }, Tank_Rotation_Factor{ 50.0f };

float Light_Revolution_Angle{}, Light_Revolution_Factor{ 50.0f };
glm::vec3 Light_Trasform(0.0f, 5.5f, 20.0f);
int light_color_template_index{};
std::vector<glm::vec3> light_color_template = {
	glm::vec3(1.0f, 1.0f, 1.0f), // White
	glm::vec3(1.0f, 0.5f, 0.5f), // Red
	glm::vec3(0.5f, 1.0f, 0.5f), // Green
	glm::vec3(0.5f, 0.5f, 1.0f), // Blue
};
GLuint ShininessID{};
//float g_shininess{ 32.0f };

int Camera_Rotation_Mode{};
glm::vec3 Camera_Rotation_Angle(0.0, 0.0, 0.0), Camera_Rotation_Factor(0.0, 15.0, 0.0);

std::vector<Vertex_glm> Axis_Vertex = {
	// Positions					// Colors
	{ glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) }, // X axis - Red
	{ glm::vec3(15.0f, 0.0f, 0.0f),  glm::vec3(1.0f, 0.0f, 0.0f) },
	{ glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) }, // Y axis - Green
	{ glm::vec3(0.0f, 15.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
	{ glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) }, // Z axis - Blue
	{ glm::vec3(0.0f, 0.0f, 15.0f), glm::vec3(0.0f, 0.0f, 1.0f) }
};
std::vector<unsigned int> Axis_Index = {
	0, 1,
	2, 3,
	4, 5
};

// 객체의 정점 데이터를 기반으로 AABB를 계산합니다.
AABB CalculateAABB(const std::vector<Vertex_glm>& vertices) {
	if (vertices.empty()) {
		return { glm::vec3(0.0f), glm::vec3(0.0f) };
	}

	AABB aabb;
	aabb.min = vertices[0].position;
	aabb.max = vertices[0].position;

	for (const auto& vertex : vertices) {
		aabb.min.x = std::min(aabb.min.x, vertex.position.x);
		aabb.min.y = std::min(aabb.min.y, vertex.position.y);
		aabb.min.z = std::min(aabb.min.z, vertex.position.z);

		aabb.max.x = std::max(aabb.max.x, vertex.position.x);
		aabb.max.y = std::max(aabb.max.y, vertex.position.y);
		aabb.max.z = std::max(aabb.max.z, vertex.position.z);
	}
	return aabb;
}

// 변환 행렬을 적용하여 AABB를 월드 좌표계로 변환합니다.
AABB TransformAABB(const AABB& aabb, const glm::mat4& transform) {
	glm::vec3 corners[8] = {
		glm::vec3(aabb.min.x, aabb.min.y, aabb.min.z),
		glm::vec3(aabb.max.x, aabb.min.y, aabb.min.z),
		glm::vec3(aabb.min.x, aabb.max.y, aabb.min.z),
		glm::vec3(aabb.max.x, aabb.max.y, aabb.min.z),
		glm::vec3(aabb.min.x, aabb.min.y, aabb.max.z),
		glm::vec3(aabb.max.x, aabb.min.y, aabb.max.z),
		glm::vec3(aabb.min.x, aabb.max.y, aabb.max.z),
		glm::vec3(aabb.max.x, aabb.max.y, aabb.max.z)
	};

	for (int i = 0; i < 8; ++i) {
		corners[i] = glm::vec3(transform * glm::vec4(corners[i], 1.0f));
	}

	AABB transformedAABB;
	transformedAABB.min = corners[0];
	transformedAABB.max = corners[0];

	for (int i = 1; i < 8; ++i) {
		transformedAABB.min.x = std::min(transformedAABB.min.x, corners[i].x);
		transformedAABB.min.y = std::min(transformedAABB.min.y, corners[i].y);
		transformedAABB.min.z = std::min(transformedAABB.min.z, corners[i].z);

		transformedAABB.max.x = std::max(transformedAABB.max.x, corners[i].x);
		transformedAABB.max.y = std::max(transformedAABB.max.y, corners[i].y);
		transformedAABB.max.z = std::max(transformedAABB.max.z, corners[i].z);
	}

	return transformedAABB;
}

// 두 AABB가 충돌하는지 확인합니다.
bool CheckCollision(const AABB& a, const AABB& b) {
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
		(a.min.y <= b.max.y && a.max.y >= b.min.y) &&
		(a.min.z <= b.max.z && a.max.z >= b.min.z);
}


// inner AABB가 outer AABB 내부에 완전히 포함되는지 확인합니다.
bool IsAABBInside(const AABB& inner, const AABB& outer) {
	return (inner.min.x >= outer.min.x && inner.max.x <= outer.max.x) &&
		(inner.min.y >= outer.min.y && inner.max.y <= outer.max.y) &&
		(inner.min.z >= outer.min.z && inner.max.z <= outer.max.z);
}


bool CheckAABBCollision(const AABB& a, const AABB& b) {
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
		(a.min.y <= b.max.y && a.max.y >= b.min.y) &&
		(a.min.z <= b.max.z && a.max.z >= b.min.z);
}
