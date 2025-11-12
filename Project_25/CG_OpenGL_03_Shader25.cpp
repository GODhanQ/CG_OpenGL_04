#define _CRT_SECURE_NO_WARNINGS
#include "VarANDFunc_03_Shader25.h"

auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
std::default_random_engine dre(seed);
std::uniform_real_distribution<float> urd_0_1(0.0f, 1.0f);
std::uniform_int_distribution<int> uid_0_3(0, 3);

glm::mat4 Perspective_Matrix(1.0f), View_Matrix;
glm::mat4 Model_Matrix(1.0f);
glm::mat4 Body_Matrix(1.0f);
glm::mat4 LeftArm_Matrix(1.0f), RightArm_Matrix(1.0f);
glm::mat4 LeftLeg_Matrix(1.0f), RightLeg_Matrix(1.0f);
glm::mat4 Door_Matrix(1.0f);

std::vector<OBJ_File> g_OBJ_Files;
std::map<std::string, AABB> g_LocalAABBs; // 객체별 로컬 AABB 저장


int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowPosition(500, 50);
	glutInitWindowSize(Window_width, Window_height);
	glutCreateWindow("Example25");

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		return 1;
	}
	std::cout << "glew initialized\n";

	shaderProgramID = make_shaderProgram("Vertex_Shader.glsl", "Fragment_Shader.glsl");
	std::cout << "Make Shader Program Completed\n";

	GetUniformLocations();
	std::cout << "Get Uniform Locations Completed\n";

	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	std::cout << "Setup GL_CULL_FACE Completed\n";

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	std::cout << "Setup GL_POLYGON_MODE Completed\n";

	INIT_BUFFER();
	std::cout << "INIT BUFFER Completed\n";

	MakeStaticMatrix();
	std::cout << "Make Static Matrix Completed\n";

	for (const auto& file : g_OBJ_Files) {
		for (const auto& object : file.objects) {
			std::cout << "Object Name: " << object.name << ", VAO: " << object.VAO << "\n";
		}
	}

	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(KeyBoard);
	glutSpecialFunc(SpecialKeyBoard);
	glutKeyboardUpFunc(KeyBoardUp);
	glutSpecialUpFunc(SpecialKeyBoardUp);
	glutMouseFunc(MouseClick);
	glutIdleFunc(drawScene);

	glutMainLoop();
}

GLvoid drawScene() {
	GLfloat rColor{ 0.5f }, gColor{ 0.5f }, bColor{ 0.7f };
	glClearColor(rColor, gColor, bColor, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderProgramID);
	MakeDynamicMatrix();

	int width = glutGet(GLUT_WINDOW_WIDTH);
	int height = glutGet(GLUT_WINDOW_HEIGHT);

	// 1. Main Viewport
	glViewport(0, 0, width, height);
	Perspective_Matrix = glm::perspective(FOV, AspectRatio, NearClip, FarClip);
	View_Matrix = glm::lookAt(EYE, AT, UP);
	UpdateUniformMatrices();

	// OBJ in Main Viewport
	// Draw Axis
	glBindVertexArray(VAO_axis);
	glUniform1i(FigureTypeID, Figure_Type::AXIS);
	glLineWidth(2.0f);
	glDrawElements(GL_LINES, Axis_Index.size(), GL_UNSIGNED_INT, 0);

	// Draw OBJ Models
	int box_face_count = 0;
	for (const auto& file : g_OBJ_Files) {
		for (const auto& object : file.objects) {
			glBindVertexArray(object.VAO);

			if (object.name == "Box") {
				glUniform1i(FigureTypeID, Figure_Type::BOX_WALL);
				glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, 0);
				glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * 24));
				glUniform1i(FigureTypeID, Figure_Type::BOX_DOOR);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * 18));
			}
			else {
				GLuint Figure_Type;
				Type_distinction(object.name, Figure_Type);
				glUniform1i(FigureTypeID, Figure_Type);

				glDrawElements(GL_TRIANGLES, object.indices.size(), GL_UNSIGNED_INT, 0);
			}
		}
	}

	// 2. Minimap Viewport
	glClear(GL_DEPTH_BUFFER_BIT);
	int minimap_size = width / 4;
	glViewport(width - minimap_size, height - minimap_size, minimap_size, minimap_size);

	// 미니맵을 위한 Orthographic Projection과 Top-down View 설정
	Perspective_Matrix = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, NearClip, FarClip);
	View_Matrix = glm::lookAt(glm::vec3(0.0f, 50.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
	UpdateUniformMatrices();

	// Re-draw objects in Minimap
	// Draw Axis
	glBindVertexArray(VAO_axis);
	glUniform1i(FigureTypeID, Figure_Type::AXIS);
	glLineWidth(2.0f);
	glDrawElements(GL_LINES, Axis_Index.size(), GL_UNSIGNED_INT, 0);

	// Draw OBJ Models
	box_face_count = 0;
	for (const auto& file : g_OBJ_Files) {
		for (const auto& object : file.objects) {
			glBindVertexArray(object.VAO);

			if (object.name == "Box") {
				glUniform1i(FigureTypeID, Figure_Type::BOX_WALL);
				glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, 0);
				glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * 24));
				glUniform1i(FigureTypeID, Figure_Type::BOX_DOOR);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * 18));
			}
			else {
				GLuint Figure_Type;
				Type_distinction(object.name, Figure_Type);
				glUniform1i(FigureTypeID, Figure_Type);

				glDrawElements(GL_TRIANGLES, object.indices.size(), GL_UNSIGNED_INT, 0);
			}
		}
	}

	glBindVertexArray(0);
	glutSwapBuffers();
}
GLvoid Reshape(int w, int h) {
	MakeStaticMatrix();
	glUniformMatrix4fv(PerspectiveMatrixID, 1, GL_FALSE, &Perspective_Matrix[0][0]);
}

void KeyBoard(unsigned char key, int x, int y) {
	keyStates[key] = true;
	switch (key) {
	case 'w':
		Model_Movement_Factor.z -= 1.0f;
		Model_Movement_Factor.z = glm::clamp(Model_Movement_Factor.z, -1.0f, 1.0f);
		break;
	case 's':
		Model_Movement_Factor.z += 1.0f;
		Model_Movement_Factor.z = glm::clamp(Model_Movement_Factor.z, -1.0f, 1.0f);
		break;
	case 'a':
		Model_Movement_Factor.x -= 1.0f;
		Model_Movement_Factor.x = glm::clamp(Model_Movement_Factor.x, -1.0f, 1.0f);
		break;
	case 'd':
		Model_Movement_Factor.x += 1.0f;
		Model_Movement_Factor.x = glm::clamp(Model_Movement_Factor.x, -1.0f, 1.0f);
		break;
	case '+':
		Model_Movement_Factor_Scale += 5.0f;
		Model_Movement_Factor_Scale = glm::clamp(Model_Movement_Factor_Scale, 10.0f, 50.0f);
		Animation_Speed += 1.0f;
		Arm_Rotation_Max_Angle += 5.0f;
		Animation_Speed = glm::clamp(Animation_Speed, 5.0f, 15.0f);
		Arm_Rotation_Max_Angle = glm::clamp(Arm_Rotation_Max_Angle, 60.0f, 90.0f);

		std::cout << "Model_Movement_Factor_Scale: " << Model_Movement_Factor_Scale << "\n";
		break;
	case '-':
		Model_Movement_Factor_Scale -= 5.0f;
		Model_Movement_Factor_Scale = glm::clamp(Model_Movement_Factor_Scale, 10.0f, 50.0f);
		Animation_Speed -= 1.0f;
		Arm_Rotation_Max_Angle -= 5.0f;
		Animation_Speed = glm::clamp(Animation_Speed, 5.0f, 15.0f);
		Arm_Rotation_Max_Angle = glm::clamp(Arm_Rotation_Max_Angle, 60.0f, 90.0f);

		std::cout << "Model_Movement_Factor_Scale: " << Model_Movement_Factor_Scale << "\n";
		break;
	case ' ':
		if (!is_Jumping) {
			is_Jumping = true;
			Model_Velocity.y = JUMP_FORCE;
		}

		std::cout << "Jump!" << (is_Jumping ? "in sky" : "on floor") << "\n";
		break;

	case 'i':
		// Reset All
		Model_Transform = glm::vec3(0.0f, 0.0f, 0.0f);
		Model_Movement_Factor = glm::vec3(0.0f, 0.0f, 0.0f);
		Model_Orientation = glm::quat();
		Model_Scale = glm::vec3(0.5f, 0.5f, 0.5f);
		Model_Movement_Factor_Scale = 10.0f;
		Animation_Time = 0.0f;
		Animation_Speed = 10.0f;
		EYE = glm::vec3(0.0f, 20.0f, 40.0f);
		Arm_Rotation_Max_Angle = 60.0f;
		Leg_Rotation_Max_Angle = 60.0f;
		is_Jumping = false;
		std::cout << "Reset All Parameters\n";
		break;

	case 'x':
		Camera_Transform_Factor.x -= 1.0f;
		Camera_Transform_Factor.x = glm::clamp(Camera_Transform_Factor.x, -1.0f, 1.0f);
		break;
	case 'X':
		Camera_Transform_Factor.x += 1.0f;
		Camera_Transform_Factor.x = glm::clamp(Camera_Transform_Factor.x, -1.0f, 1.0f);
		break;
	case 'y':
		Camera_Rotation_Factor -= 1.0f;
		Camera_Rotation_Factor = glm::clamp(Camera_Rotation_Factor, -1.0f, 1.0f);
		break;
	case 'Y':
		Camera_Rotation_Factor += 1.0f;
		Camera_Rotation_Factor = glm::clamp(Camera_Rotation_Factor, -1.0f, 1.0f);
		break;
	case 'z':
		Camera_Transform_Factor.z -= 1.0f;
		Camera_Transform_Factor.z = glm::clamp(Camera_Transform_Factor.z, -1.0f, 1.0f);
		break;
	case 'Z':
		Camera_Transform_Factor.z += 1.0f;
		Camera_Transform_Factor.z = glm::clamp(Camera_Transform_Factor.z, -1.0f, 1.0f);
		break;
	case 'o':
		OpenDoor = !OpenDoor;

		std::cout << "OpenDoor: " << (OpenDoor ? "True" : "False") << "\n";
		break;
	case 'O':
		AT = glm::vec3(0.0f, 50.0f, 0.0f);

		std::cout << "Camera AT set to Top-down View\n";
		break;

	case '1':
		LookAtRobot = !LookAtRobot;

		if (!LookAtRobot) {
			AT = glm::vec3(0.0f, 5.0f, 0.0f);
			EYE = glm::vec3(0.0f, 30.0f, 50.0f);
		}
		break;

	case 'q':
		exit(0);

	default:
		break;
	}
}
void KeyBoardUp(unsigned char key, int x, int y) {
	keyStates[key] = false;
	switch (key) {
	case 'w':
		Model_Movement_Factor.z += 1.0f;
		break;
	case 's':
		Model_Movement_Factor.z -= 1.0f;
		break;
	case 'a':
		Model_Movement_Factor.x += 1.0f;
		break;
	case 'd':
		Model_Movement_Factor.x -= 1.0f;
		break;

	case 'x':
	case 'X':
		Camera_Transform_Factor.x = 0.0f;
		break;
	case 'y':
	case 'Y':
		Camera_Rotation_Factor = 0.0f;
		break;
	case 'z':
	case 'Z':
		Camera_Transform_Factor.z = 0.0f;
		break;

	default:
		break;
	}
}
void SpecialKeyBoard(int key, int x, int y) {
	specialKeyStates[key] = true;
	switch (key) {
	default:
		break;
	}
}
void SpecialKeyBoardUp(int key, int x, int y) {
	specialKeyStates[key] = false;
	switch (key) {
	default:
		break;
	}
}
void MouseClick(int button, int state, int x, int y) {

}
std::pair<float, float> ConvertScreenToOpenGL(int screen_x, int screen_y) {
	int width = glutGet(GLUT_WINDOW_WIDTH);
	int height = glutGet(GLUT_WINDOW_HEIGHT);

	float ogl_x = (2.0f * screen_x) / width - 1.0f;
	float ogl_y = 1.0f - (2.0f * screen_y) / height;

	return { ogl_x, ogl_y };
}

void INIT_BUFFER() {
	std::vector<std::string> obj_filenames = {
		"Box.obj",
		"Robot.obj",
		"Cube.obj",
		"Test.obj",
	};

	for (const auto& filename : obj_filenames) {
		OBJ_File objFile;
		if (ReadObj(filename, objFile)) {
			std::cout << "Loaded OBJ File: " << filename << " with " << objFile.objects.size() << " objects.\n";
			g_OBJ_Files.push_back(objFile);
		}
		else {
			std::cerr << "Failed to load OBJ File: " << filename << "\n";
		}
	}
	ComposeOBJColor();
	for (auto& file : g_OBJ_Files) {
		for (auto& object : file.objects) {
			glGenVertexArrays(1, &object.VAO);
			glGenBuffers(1, &object.VBO);
			glGenBuffers(1, &object.IBO);

			glBindVertexArray(object.VAO);

			glBindBuffer(GL_ARRAY_BUFFER, object.VBO);
			glBufferData(GL_ARRAY_BUFFER, object.vertices.size() * sizeof(Vertex_glm), object.vertices.data(), GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_glm), (GLvoid*)offsetof(Vertex_glm, position));
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_glm), (GLvoid*)offsetof(Vertex_glm, color));
			glEnableVertexAttribArray(1);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.IBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, object.indices.size() * sizeof(unsigned int), object.indices.data(), GL_STATIC_DRAW);

			std::cout << "  L  Buffered Object: " << object.name << ", VAO: " << object.VAO << "\n";

			// 각 객체의 로컬 AABB 계산 및 저장
			g_LocalAABBs[object.name] = CalculateAABB(object.vertices);
			std::cout << "     Calculated AABB for " << object.name << "\n";
		}
	}
	glBindVertexArray(0);


	std::cout << "Setup Axis...\n";
	glGenVertexArrays(1, &VAO_axis);
	glGenBuffers(1, &VBO_axis);
	glGenBuffers(1, &IBO_axis);

	glBindVertexArray(VAO_axis);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_axis);

	glBufferData(GL_ARRAY_BUFFER, Axis_Vertex.size() * sizeof(Vertex_glm), Axis_Vertex.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_glm), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_glm), (GLvoid*)offsetof(Vertex_glm, color));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO_axis);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, Axis_Index.size() * sizeof(unsigned int), Axis_Index.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

char* filetobuf(const char* file)
{
	FILE* fptr;
	long length;
	char* buf;
	fptr = fopen(file, "rb");						// Open file for reading
	if (!fptr)										// Return NULL on failure
		return NULL;
	fseek(fptr, 0, SEEK_END);						// Seek to the end of the file
	length = ftell(fptr);							// Find out how many bytes into the file we are
	buf = (char*)malloc(length + 1);				// Allocate a buffer for the entire length of the file and a null terminator
	fseek(fptr, 0, SEEK_SET);						// Go back to the beginning of the file
	fread(buf, length, 1, fptr);					// Read the contents of the file in to the buffer
	fclose(fptr);									// Close the file
	buf[length] = 0;								// Null terminator
	return buf;										// Return the buffer
}
GLuint make_shaderProgram(const char* vertPath, const char* fragPath) {
	GLchar* vertexSource;
	GLchar* fragmentSource;

	vertexSource = filetobuf(vertPath);
	fragmentSource = filetobuf(fragPath);

	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	free(vertexSource);
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		std::cerr << "ERROR: vertex shader Compile Failed (" << vertPath << ")\n" << errorLog << std::endl;
		return 0;
	}

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);
	free(fragmentSource);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		std::cerr << "ERROR: frag_shader Compile Failed (" << fragPath << ")\n" << errorLog << std::endl;
		return 0;
	}

	GLuint shaderID;
	shaderID = glCreateProgram();
	glAttachShader(shaderID, vertexShader);
	glAttachShader(shaderID, fragmentShader);
	glLinkProgram(shaderID);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	glGetProgramiv(shaderID, GL_LINK_STATUS, &result);
	if (!result) {
		glGetProgramInfoLog(shaderID, 512, NULL, errorLog);
		std::cerr << "ERROR: shader program link Failed\n" << errorLog << std::endl;
		return 0;
	}
	return shaderID;
}
bool ReadObj(const std::string& path, OBJ_File& outFile) {
	outFile.file_name = path;
	outFile.objects.clear();
	std::string real_path = "OBJ/" + path;


	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;

	FILE* file = fopen(real_path.c_str(), "r");
	if (file == NULL) {
		printf("Impossible to open the file !\n");
		return false;
	}

	Custom_OBJ* currentObject = nullptr;

	while (1) {
		char lineHeader[128];
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF) {
			break;
		}

		if (strcmp(lineHeader, "o") == 0) {
			char objectName[128];
			fscanf(file, "%s\n", objectName);
			outFile.objects.emplace_back();
			currentObject = &outFile.objects.back();
			currentObject->name = objectName;
		}
		else if (strcmp(lineHeader, "v") == 0) {
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			if (currentObject == nullptr) {
				// 'o' 태그 없이 'f'가 먼저 나오는 경우를 대비해 기본 객체 생성
				outFile.objects.emplace_back();
				currentObject = &outFile.objects.back();
				currentObject->name = "default_object";
			}

			unsigned int vertexIndex[4], uvIndex[4], normalIndex[4];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d\n",
				&vertexIndex[0], &uvIndex[0], &normalIndex[0],
				&vertexIndex[1], &uvIndex[1], &normalIndex[1],
				&vertexIndex[2], &uvIndex[2], &normalIndex[2],
				&vertexIndex[3], &uvIndex[3], &normalIndex[3]);

			auto processFace = [&](int count) {
				for (int i = 0; i < count; ++i) {
					Vertex_glm vertex;
					glm::vec3 normal;
					vertex.position = temp_vertices[vertexIndex[i] - 1];
					// UV, Normal 정보가 있다면 여기에 추가할 수 있습니다.
					// vertex.uv = temp_uvs[uvIndex[i] - 1];
					normal = temp_normals[normalIndex[i] - 1];
					vertex.color = glm::vec3(1.0f, 1.0f, 1.0f); // 기본 색상

					currentObject->vertices.push_back(vertex);
					currentObject->normals.push_back(normal);
					currentObject->indices.push_back(currentObject->vertices.size() - 1);
				}
				};

			if (matches == 9) { // 삼각형
				processFace(3);
			}
			else if (matches == 12) { // 사각형 -> 삼각형 2개로 분할
				unsigned int v_indices[] = { 0, 1, 2, 0, 2, 3 };
				for (int i = 0; i < 6; ++i) {
					int idx = v_indices[i];
					Vertex_glm vertex;
					vertex.position = temp_vertices[vertexIndex[idx] - 1];
					vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
					currentObject->vertices.push_back(vertex);
					currentObject->indices.push_back(currentObject->vertices.size() - 1);
				}
			}
			else {
				// 다른 형식의 면 데이터는 현재 파서에서 지원하지 않음
				char buffer[1024];
				fgets(buffer, 1024, file); // 해당 라인의 나머지를 읽고 무시
			}
		}
		else {
			// 주석 또는 지원되지 않는 라인 스킵
			char buffer[1024];
			fgets(buffer, 1024, file);
		}
	}

	fclose(file);
	return true;
}

void MakeStaticMatrix() {
	Perspective_Matrix = glm::perspective(FOV, AspectRatio, NearClip, FarClip);
}
void MakeDynamicMatrix() {
	// delta time
	static auto lastTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
	lastTime = currentTime;

	// Camera Translation
	EYE += Camera_Transform_Factor * deltaTime * Camera_Movement_Factor_Scale;

	// Camera Rotation
	if (Camera_Rotation_Factor != 0.0f) {
		glm::vec3 direction = EYE - AT;
		glm::mat4 rotation_matrix = glm::rotate(glm::mat4(1.0f),
			glm::radians(Camera_Rotation_Factor * Camera_Rotation_Factor_Scale * deltaTime),
			glm::vec3(0.0f, 1.0f, 0.0f));
		EYE = AT + glm::vec3(rotation_matrix * glm::vec4(direction, 0.0f));
	}

	View_Matrix = glm::lookAt(EYE, AT, UP);

	// Box Door Animation
	if (OpenDoor) {
		Door_Open_Progress += doorAnimationSpeed * deltaTime;
	}
	else {
		Door_Open_Progress -= doorAnimationSpeed * deltaTime;
	}
	Door_Open_Progress = glm::clamp(Door_Open_Progress, 0.0f, 1.0f);

	glm::vec3 doorSlideOffset = glm::vec3(0.0f, doorSlideHeight * Door_Open_Progress, 0.0f);
	Door_Matrix = glm::translate(glm::mat4(1.0f), doorSlideOffset);

	// 1. 중력 적용
	Model_Velocity.y -= GRAVITY * deltaTime;

	// 2. 사용자 입력에 따른 수평 이동 계산
	glm::vec3 horizontal_move(0.0f);
	if (glm::length(Model_Movement_Factor) > 0.0f) {
		// Animation
		Animation_Time += deltaTime * Animation_Speed;
		Arm_Rotation_Angle.x = Arm_Rotation_Max_Angle * sin(Animation_Time);
		Leg_Rotation_Angle.x = Leg_Rotation_Max_Angle * cos(Animation_Time);

		glm::vec3 move_direction = glm::normalize(Model_Movement_Factor);
		glm::quat target_orientation = glm::quatLookAt(move_direction, glm::vec3(0.0f, 1.0f, 0.0f));

		Model_Orientation = glm::slerp(Model_Orientation, target_orientation, deltaTime * Rotation_Speed);

		horizontal_move = Model_Orientation * glm::vec3(0, 0, -1) * Model_Movement_Factor_Scale * deltaTime;
	}
	else {
		Animation_Time = 0.0f;
		Arm_Rotation_Angle.x = 0.0f;
		Leg_Rotation_Angle.x = 0.0f;
	}

	// 3. 충돌 감지 및 반응 (축 분리)
	// Box는 월드 고정(Identity)로 가정
	AABB boxWorldAABB = TransformAABB(g_LocalAABBs["Box"], glm::mat4(1.0f));

	// 로봇 파트 목록 (로봇 자체는 장애물 대상에서 제외)
	std::vector<std::string> robot_parts = { "body", "left_arm", "right_arm", "left_leg", "right_leg" };

	// 장애물 목록 구성: g_LocalAABBs의 모든 항목 중 로봇 파트는 제외 (고정 장애물, world transform = identity)
	std::vector<std::pair<std::string, AABB>> obstacles;
	for (const auto& kv : g_LocalAABBs) {
		const std::string& name = kv.first;
		if (std::find(robot_parts.begin(), robot_parts.end(), name) != robot_parts.end()) continue;
		glm::mat4 obstacleWorldMat = glm::mat4(1.0f);
		AABB worldAABB = TransformAABB(kv.second, obstacleWorldMat);
		obstacles.emplace_back(name, worldAABB);
	}

	glm::mat4 rotationMatrix = glm::mat4_cast(Model_Orientation);
	glm::mat4 correctionMatrix = glm::rotate(glm::mat4(1.0f),
		glm::radians(180.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));

	// 3-1. 수평 이동 (XZ)
	glm::vec3 next_pos_h = Model_Transform + glm::vec3(horizontal_move.x, 0.0f, horizontal_move.z);
	glm::mat4 nextModelMatrix_h = glm::translate(glm::mat4(1.0f), next_pos_h);
	nextModelMatrix_h = glm::scale(nextModelMatrix_h, Model_Scale);
	nextModelMatrix_h = nextModelMatrix_h * rotationMatrix * correctionMatrix;

	bool horizontalCollision = false;
	for (const auto& part_name : robot_parts) {
		if (!g_LocalAABBs.count(part_name)) continue;
		AABB robotPartWorldAABB = TransformAABB(g_LocalAABBs[part_name], nextModelMatrix_h);

		// (A) Box 규칙: 로봇 파트는 Box 내부에 있어야 함 — 벗어나면 충돌
		if (!IsAABBInside(robotPartWorldAABB, boxWorldAABB)) {
			horizontalCollision = true;
			break;
		}

		// (B) 다른 고정 장애물과의 충돌 검사 (filled)
		for (const auto& obs : obstacles) {
			if (obs.first == "Box") continue;
			if (CheckCollision(robotPartWorldAABB, obs.second)) {
				horizontalCollision = true;
				Model_Movement_Factor = -Model_Movement_Factor * 0.1f;
				break;
			}
		}
		if (horizontalCollision) break;
	}

	if (!horizontalCollision) {
		Model_Transform = next_pos_h;
	}

	// 3-2. 수직 이동 (Y) — 단일 블록만 사용
	glm::vec3 vertical_move_vec = glm::vec3(0.0f, Model_Velocity.y * deltaTime, 0.0f);
	glm::vec3 next_pos_v = Model_Transform + vertical_move_vec;
	glm::mat4 nextModelMatrix_v = glm::translate(glm::mat4(1.0f), next_pos_v);
	nextModelMatrix_v = glm::scale(nextModelMatrix_v, Model_Scale);
	nextModelMatrix_v = nextModelMatrix_v * rotationMatrix * correctionMatrix;

	bool verticalCollision = false;
	for (const auto& part_name : robot_parts) {
		if (!g_LocalAABBs.count(part_name)) continue;
		AABB robotPartWorldAABB = TransformAABB(g_LocalAABBs[part_name], nextModelMatrix_v);

		// Box 내부 체크 (원래 로직 유지)
		if (!IsAABBInside(robotPartWorldAABB, boxWorldAABB)) {
			verticalCollision = true;
			break;
		}

		// 다른 장애물과의 교차 체크
		for (const auto& obs : obstacles) {
			if (obs.first == "Box") continue;
			if (CheckCollision(robotPartWorldAABB, obs.second)) {
				verticalCollision = true;
				break;
			}
		}
		if (verticalCollision) break;
	}

	if (verticalCollision) {
		// 바닥 또는 천장과 충돌 처리 (Box 기반 보정 유지)
		if (Model_Velocity.y < 0) { // 바닥 충돌
			if (g_LocalAABBs.count("left_leg")) {
				//Model_Transform.y = boxWorldAABB.min.y - (g_LocalAABBs["left_leg"].min.y * Model_Scale.y);
			}
			Model_Velocity.y = 0.0f;
			is_Jumping = false;
		}
		else if (Model_Velocity.y > 0) { // 천장 충돌
			Model_Velocity.y = 0.0f;
		}
	}
	else {
		Model_Transform = next_pos_v;
		is_Jumping = true;
	}

	rotationMatrix = glm::mat4_cast(Model_Orientation);
	correctionMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	Model_Matrix = glm::mat4(1.0f);
	Model_Matrix = glm::translate(Model_Matrix, Model_Transform);
	Model_Matrix = glm::scale(Model_Matrix, Model_Scale);
	Model_Matrix = Model_Matrix * rotationMatrix * correctionMatrix;

	if (LookAtRobot) {
		AT = Model_Transform;
		EYE = AT + glm::vec3(0.0f, 15.0f, 20.0f);
	}

	LeftArm_Matrix = glm::translate(glm::mat4(1.0f), Arm_Offset);
	LeftArm_Matrix = glm::rotate(LeftArm_Matrix, glm::radians(Arm_Rotation_Angle.x), glm::vec3(1.0f, 0.0f, 0.0f));
	LeftArm_Matrix = glm::translate(LeftArm_Matrix, -Arm_Offset);

	RightArm_Matrix = glm::translate(glm::mat4(1.0f), glm::vec3(-Arm_Offset.x, Arm_Offset.y, Arm_Offset.z));
	RightArm_Matrix = glm::rotate(RightArm_Matrix, glm::radians(-Arm_Rotation_Angle.x), glm::vec3(1.0f, 0.0f, 0.0f));
	RightArm_Matrix = glm::translate(RightArm_Matrix, -glm::vec3(-Arm_Offset.x, Arm_Offset.y, Arm_Offset.z));

	LeftLeg_Matrix = glm::translate(glm::mat4(1.0f), Leg_Offset);
	LeftLeg_Matrix = glm::rotate(LeftLeg_Matrix, glm::radians(Leg_Rotation_Angle.x), glm::vec3(1.0f, 0.0f, 0.0f));
	LeftLeg_Matrix = glm::translate(LeftLeg_Matrix, -Leg_Offset);

	RightLeg_Matrix = glm::translate(glm::mat4(1.0f), glm::vec3(-Leg_Offset.x, Leg_Offset.y, Leg_Offset.z));
	RightLeg_Matrix = glm::rotate(RightLeg_Matrix, glm::radians(-Leg_Rotation_Angle.x), glm::vec3(1.0f, 0.0f, 0.0f));
	RightLeg_Matrix = glm::translate(RightLeg_Matrix, -glm::vec3(-Leg_Offset.x, Leg_Offset.y, Leg_Offset.z));
}

void GetUniformLocations() {
	// static uniform variable
	PerspectiveMatrixID = glGetUniformLocation(shaderProgramID, "Perspective_Matrix");
	ViewMatrixID = glGetUniformLocation(shaderProgramID, "View_Matrix");
	ModelMatrixID = glGetUniformLocation(shaderProgramID, "Model_Matrix");
	BodyMatrixID = glGetUniformLocation(shaderProgramID, "Body_Matrix");
	LeftArmMatrixID = glGetUniformLocation(shaderProgramID, "LeftArm_Matrix");
	RightArmMatrixID = glGetUniformLocation(shaderProgramID, "RightArm_Matrix");
	LeftLegMatrixID = glGetUniformLocation(shaderProgramID, "LeftLeg_Matrix");
	RightLegMatrixID = glGetUniformLocation(shaderProgramID, "RightLeg_Matrix");
	DoorMatrixID = glGetUniformLocation(shaderProgramID, "Door_Matrix");

	// dynamic uniform variable
	FigureTypeID = glGetUniformLocation(shaderProgramID, "Figure_Type");
}
void UpdateUniformMatrices() {
	glUniformMatrix4fv(PerspectiveMatrixID, 1, GL_FALSE, &Perspective_Matrix[0][0]);
	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &View_Matrix[0][0]);
	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Model_Matrix[0][0]);
	glUniformMatrix4fv(BodyMatrixID, 1, GL_FALSE, &Body_Matrix[0][0]);
	glUniformMatrix4fv(LeftArmMatrixID, 1, GL_FALSE, &LeftArm_Matrix[0][0]);
	glUniformMatrix4fv(RightArmMatrixID, 1, GL_FALSE, &RightArm_Matrix[0][0]);
	glUniformMatrix4fv(LeftLegMatrixID, 1, GL_FALSE, &LeftLeg_Matrix[0][0]);
	glUniformMatrix4fv(RightLegMatrixID, 1, GL_FALSE, &RightLeg_Matrix[0][0]);
	glUniformMatrix4fv(DoorMatrixID, 1, GL_FALSE, &Door_Matrix[0][0]);


	if (PerspectiveMatrixID == -1) std::cerr << "Could not bind uniform Perspective_Matrix\n";
	if (ViewMatrixID == -1) std::cerr << "Could not bind uniform View_Matrix\n";
	if (ModelMatrixID == -1) std::cerr << "Could not bind uniform Model_Matrix\n";
	if (BodyMatrixID == -1) std::cerr << "Could not bind uniform Body_Matrix\n";
	if (LeftArmMatrixID == -1) std::cerr << "Could not bind uniform LeftArm_Matrix\n";
	if (RightArmMatrixID == -1) std::cerr << "Could not bind uniform RightArm_Matrix\n";
	if (LeftLegMatrixID == -1) std::cerr << "Could not bind uniform LeftLeg_Matrix\n";
	if (RightLegMatrixID == -1) std::cerr << "Could not bind uniform RightLeg_Matrix\n";
	if (DoorMatrixID == -1) std::cerr << "Could not bind uniform Door_Matrix\n";


}
void ComposeOBJColor() {
	for (auto& file : g_OBJ_Files) {
		for (auto& object : file.objects) {
			if (object.name == "Box") {
				glm::vec3 faceColors[6] = {
					glm::vec3(0.6f, 0.6f, 0.6f),
					glm::vec3(0.2f, 0.2f, 0.2f),
					glm::vec3(0.6f, 0.6f, 0.6f),
					glm::vec3(0.2f, 0.2f, 0.2f),
					glm::vec3(0.4f, 0.4f, 0.4f),
					glm::vec3(0.4f, 0.4f, 0.4f)
				};

				int trianglePerFace = 6;
				for (size_t i = 0; i < object.indices.size(); ++i) {
					unsigned int faceIndex = i / trianglePerFace;
					faceIndex = faceIndex % 6;
					object.vertices[object.indices[i]].color = faceColors[faceIndex];
				}
			}
			else if (object.name == "body") {
				glm::vec3 bodyColor(0.5f, 0.5f, 0.8f);
				for (auto& vertex : object.vertices) {
					vertex.color = bodyColor;
				}
			}
			else if (object.name == "left_arm" || object.name == "right_arm") {
				glm::vec3 armColor(0.3f, 0.3f, 0.8f);
				for (auto& vertex : object.vertices) {
					vertex.color = armColor;
				}
			}
			else if (object.name == "left_leg" || object.name == "right_leg") {
				glm::vec3 legColor(0.8f, 0.3f, 0.3f);
				for (auto& vertex : object.vertices) {
					vertex.color = legColor;
				}
			}
			else {
				// 기타 객체에 대해 무작위 색상 할당
				glm::vec3 randomColor(urd_0_1(dre), urd_0_1(dre), urd_0_1(dre));
				for (auto& vertex : object.vertices) {
					vertex.color = randomColor;
				}
			}
		}
	}
}

void Type_distinction(const std::string& object_name, GLuint& outTypeID) {
	if (object_name == "body") {
		outTypeID = Figure_Type::BODY;
	}
	else if (object_name == "left_arm") {
		outTypeID = Figure_Type::LEFT_ARM;
	}
	else if (object_name == "right_arm") {
		outTypeID = Figure_Type::RIGHT_ARM;
	}
	else if (object_name == "left_leg") {
		outTypeID = Figure_Type::LEFT_LEG;
	}
	else if (object_name == "right_leg") {
		outTypeID = Figure_Type::RIGHT_LEG;
	}
	else if (object_name == "Box") {
		outTypeID = Figure_Type::BOX;
	}
	else {
		outTypeID = Figure_Type::ETC;
	}
}