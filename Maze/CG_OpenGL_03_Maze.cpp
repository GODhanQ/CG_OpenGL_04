#define _CRT_SECURE_NO_WARNINGS
#include "VarANDFunc_03_Maze.h"

auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
std::default_random_engine dre(seed);
std::uniform_real_distribution<float> urd_0_1(0.0f, 1.0f);
std::uniform_int_distribution<int> uid_0_3(0, 3);

glm::mat4 Perspective_Matrix(1.0f), View_Matrix(1.0f);
glm::mat4 Model_Matrix(1.0f);
glm::mat4 Floor_Matrix(1.0f), Tank_Matrix(1.0f);
glm::mat4 Wall_Matrix(1.0f);
glm::mat4 Robot_Matrix(1.0f);

std::vector<OBJ_File> g_OBJ_Files;
std::map<std::string, AABB> g_LocalAABBs;

GLuint VAO_orbit, VBO_orbit, IBO_orbit;;
std::vector<Light> g_Lights;
std::vector<Vertex_glm> orbit_vertices;
std::vector<unsigned int> orbit_indices;

Maze g_Maze(0, 0);

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(Window_width, Window_height);
	glutCreateWindow("Maze");

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

	//g_Maze = MakeMaze(9, 9);

	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(KeyBoard);
	glutSpecialFunc(SpecialKeyBoard);
	glutKeyboardUpFunc(KeyBoardUp);
	glutSpecialUpFunc(SpecialKeyBoardUp);
	glutMouseFunc(MouseClick);
	glutMotionFunc(MouseMotion);
	glutPassiveMotionFunc(MouseMotion);
	glutIdleFunc(drawScene);

	glutMainLoop();
}

GLvoid drawScene() {
	GLfloat rColor{ 0.2f }, gColor{ 0.2f }, bColor{ 0.4f };
	glClearColor(rColor, gColor, bColor, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	static auto lastTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
	lastTime = currentTime;

	glUseProgram(shaderProgramID);
	MakeDynamicMatrix(deltaTime);

	int width = glutGet(GLUT_WINDOW_WIDTH);
	int height = glutGet(GLUT_WINDOW_HEIGHT);

	// 1. Main Viewport
	glViewport(0, 0, width, height);
	if (Perspective_On)	Perspective_Matrix = glm::perspective(FOV, AspectRatio, NearClip, FarClip);
	else Perspective_Matrix = glm::ortho(-50.0f, 50.0f, -50.0f * (float)height / (float)width, 50.0f * (float)height / (float)width, NearClip, FarClip);

	UpdateUniformMatrices();
	DrawModels(deltaTime);

	// 2. Minimap Viewport
	int minimap_size = std::min(width, height) / 4;
	glViewport(width - minimap_size - 10, height - minimap_size - 10, minimap_size, minimap_size);

	AABB wallAABB = g_LocalAABBs.at("Wall");
	glm::vec3 wallSize = wallAABB.max - wallAABB.min;

	float maze_world_width = g_Maze.width * wallSize.x;
	float maze_world_height = g_Maze.height * wallSize.z;
	float maze_max_dimension = std::max(maze_world_width, maze_world_height);

	float camera_height = maze_max_dimension * 1.5f;

	glm::vec3 tempEYE = glm::vec3(0.0f, camera_height, 0.01f);
	View_Matrix = glm::lookAt(tempEYE, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));

	float ortho_size = maze_max_dimension * 0.65f;
	Perspective_Matrix = glm::ortho(-ortho_size, ortho_size, -ortho_size, ortho_size, 0.1f, camera_height * 2.0f);

	UpdateUniformMatrices();
	DrawModels(deltaTime);

	// Red dot at the Robot position on minimap
	if (RobotInWorld) {
		glUseProgram(0);
		glDisable(GL_DEPTH_TEST);

		glm::vec3 robot_world_pos = Robot_Position;

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(&Perspective_Matrix[0][0]);

		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(&View_Matrix[0][0]);

		glPointSize(5.0f);
		glBegin(GL_POINTS);
		glColor3f(1.0f, 0.0f, 0.0f);
		glVertex3f(robot_world_pos.x, 15.0f, robot_world_pos.z);
		glEnd();

		glEnable(GL_DEPTH_TEST);
		glUseProgram(shaderProgramID);
	}

	// 3. Display Parameters (전체 화면 기준)
	glViewport(0, 0, width, height);
	DisplayParameters(deltaTime);

	glBindVertexArray(0);
	glutSwapBuffers();
}
GLvoid Reshape(int w, int h) {
	MakeStaticMatrix();
	glUniformMatrix4fv(PerspectiveMatrixID, 1, GL_FALSE, &Perspective_Matrix[0][0]);
}
void DrawModels(float deltaTime) {
	// Draw Axis
	glBindVertexArray(VAO_axis);
	glUniform1i(FigureTypeID, Figure_Type::AXIS);
	glLineWidth(2.0f);
	glDrawElements(GL_LINES, Axis_Index.size(), GL_UNSIGNED_INT, 0);

	// Light Orbit Path
	glBindVertexArray(VAO_orbit);
	glUniform1i(FigureTypeID, Figure_Type::ORBIT);
	glLineWidth(1.0f);
	glDrawElements(GL_LINE_STRIP, orbit_indices.size(), GL_UNSIGNED_INT, 0);

	// Draw Light Source & Set Light Uniforms
	if (Light_On) {
		glUniform1i(glGetUniformLocation(shaderProgramID, "numLights"), g_Lights.size());
		for (int i = 0; i < g_Lights.size(); ++i) {
			auto& light = g_Lights[i];

			// Uniforms 설정
			glm::vec3 lightPos_world = light.light_vertex.position;
			std::string baseName = "lights[" + std::to_string(i) + "]";
			glUniform3fv(glGetUniformLocation(shaderProgramID, (baseName + ".position").c_str()), 1, &lightPos_world[0]);
			glUniform3fv(glGetUniformLocation(shaderProgramID, (baseName + ".color").c_str()), 1, &light.light_color[0]);
			glUniform1f(glGetUniformLocation(shaderProgramID, (baseName + ".constant").c_str()), light.constant);
			glUniform1f(glGetUniformLocation(shaderProgramID, (baseName + ".linear").c_str()), light.linear);
			glUniform1f(glGetUniformLocation(shaderProgramID, (baseName + ".quadratic").c_str()), light.quadratic);
			glUniform1f(glGetUniformLocation(shaderProgramID, (baseName + ".intensity").c_str()), light.intensity);

			// 광원 그리기
			glBindVertexArray(light.VAO);
			glUniform1i(FigureTypeID, Figure_Type::LIGHT);
			glPointSize(5.0f);
			glDrawElements(GL_POINTS, 1, GL_UNSIGNED_INT, 0); 
		}
	}
	else {
		glUniform1i(glGetUniformLocation(shaderProgramID, "numLights"), 0);
	}

	// Draw Maze Walls
	const Custom_OBJ* wallObject = nullptr;
	for (const auto& file : g_OBJ_Files) {
		for (const auto& object : file.objects) {
			if (object.name == "Wall") {
				wallObject = &object;
				break;
			}
		}
		if (wallObject) break;
	}

	if (wallObject) {
		glBindVertexArray(wallObject->VAO);
		glUniform1f(ShininessID, wallObject->shininess);

		GLuint wallFigureType;
		Type_distinction("Wall", wallFigureType);
		glUniform1i(FigureTypeID, wallFigureType);

		AABB wallAABB = g_LocalAABBs.at("Wall");
		glm::vec3 wallSize = wallAABB.max - wallAABB.min;

		const float pi = 3.141592f;

		for (int i = 0; i < g_Maze.height; ++i) {
			for (int j = 0; j < g_Maze.width; ++j) {
				if (g_Maze.grid[i][j] == 1) {
					glm::mat4 wallModelMatrix = glm::mat4(1.0f);
					float x_offset = -g_Maze.width * wallSize.x / 2.0f;
					float z_offset = -g_Maze.height * wallSize.z / 2.0f;

					float& theta = g_Maze.wallHeights[i][j];

					if (ScaleWallHeight) {
						float animation_speed{};
						if (!CreatingWalls) animation_speed = 5.0f * urd_0_1(dre) * animation_speed_factor;
						else animation_speed = 0.5f * animation_speed_factor;
						theta += animation_speed * deltaTime;
						if (theta > 2.0f * pi) {
							theta -= 2.0f * pi;
							if (CreatingWalls) ScaleWallHeight = false;
							CreatingWalls = false;
						}
					}

					float cos_value = cos(theta);
					float normalized_height = (cos_value + 1.0f) * 0.5f;

					float height_scale = 1.0f + (WallMaxHeight - 1.0f) * normalized_height;
					
					glm::vec3 scale = glm::vec3(1.0f, height_scale, 1.0f);
					glm::vec3 position(j * wallSize.x + x_offset, 0.0f, i * wallSize.z + z_offset);
					scale *= 1.0f;
					wallModelMatrix = glm::scale(wallModelMatrix, scale);
					wallModelMatrix = glm::translate(wallModelMatrix, position);

					glUniformMatrix4fv(WallMatrixID, 1, GL_FALSE, &wallModelMatrix[0][0]);
					glDrawElements(GL_TRIANGLES, wallObject->indices.size(), GL_UNSIGNED_INT, 0);
				}
			}
		}
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Model_Matrix[0][0]);
	}

	// Draw OBJ Models
	for (const auto& file : g_OBJ_Files) {
		if (file.file_name == "Robot.obj" && !RobotInWorld) {
			continue;
		}
		for (const auto& object : file.objects) {
			if (!object.name.find("Tank") ||
				!object.name.find("Wall")) {
				continue;
			}
			glBindVertexArray(object.VAO);

			GLuint currentFigureType;
			Type_distinction(object.name, currentFigureType);
			glUniform1i(FigureTypeID, currentFigureType);

			glUniform1f(ShininessID, object.shininess);

			glDrawElements(GL_TRIANGLES, object.indices.size(), GL_UNSIGNED_INT, 0);
		}
	}
}

void KeyBoard(unsigned char key, int x, int y) {
	keyStates[key] = true;
	switch (key) {
	case 'c':
		// Reset All Parameters
		EYE = glm::vec3(0.0f, 30.0f, 50.0f);
		AT = glm::vec3(0.0f, 10.0f, 0.0f);
		UP = glm::vec3(0.0f, 1.0f, 0.0f);
		View_Matrix = glm::lookAt(EYE, AT, UP);
		Rotation_Mode = 0;
		Revolution_Mode = 0;
		Camera_Rotation_Mode = 0;
		ScaleWallHeight = false;
		g_Maze = Maze(1, 1);
		g_Lights.clear();
		MakeLightSources();
		Perspective_On = true;
		RobotInWorld = false;

		std::cout << "Reset Camera and Rotation Parameters\n";
		break;
	case 'm':
		ScaleWallHeight = !ScaleWallHeight;

		std::cout << "Wall Height Animation: " << (ScaleWallHeight ? "ON\n" : "OFF\n");
		break;
	case 'v': {
		const float pi = 3.141592f;
		for (int i = 0; i < g_Maze.height; ++i) {
			for (int j = 0; j < g_Maze.width; ++j) {
				g_Maze.wallHeights[i][j] = pi;
			}
		}
		ScaleWallHeight = false;
		std::cout << "Reset all wall heights to 1.0\n";
		break;
	}
	case 'j':
		Revolution_Mode = (Revolution_Mode + 1) % 3;

		if (Revolution_Mode == 0)
			std::cout << "Revolution Stopped\n";
		else if (Revolution_Mode == 1)
			std::cout << "Revolution +Y Axis\n";
		else if (Revolution_Mode == 2)
			std::cout << "Revolution -Y Axis\n";
		break;
	case 'l':
		for (auto& light : g_Lights) {
			float original_y = light.init_position.y;
			light.init_position = 0.8f * light.init_position;
			light.init_position.y = original_y;
			light.light_vertex.position = light.init_position;
		}

		ComposeOribit();
		break;
	case 'L':
		for (auto& light : g_Lights) {
			float original_y = light.init_position.y;
			light.init_position = 1.2f * light.init_position;
			light.init_position.y = original_y;
			light.light_vertex.position = light.init_position;
		}

		ComposeOribit();
		break;
	case '.':
		light_color_template_index = (light_color_template_index + 1) % light_color_template.size();

		for (auto& light : g_Lights) {
			light.light_color = light_color_template[light_color_template_index];
			light.light_vertex.color = light.light_color;
		}

		break;

	case 'k':
		for (auto& file : g_OBJ_Files) {
			if (file.file_name != "Robot.obj") continue;
			for (auto& object : file.objects) {
				object.shininess *= 2.0f;
				std::cout << "Robot Shininess: " << object.shininess << std::endl;
			}
		}
		break;
	case 'K':
		for (auto& file : g_OBJ_Files) {
			if (file.file_name != "Robot.obj") continue;
			for (auto& object : file.objects) {
				object.shininess /= 2.0f;
				std::cout << "Robot Shininess: " << object.shininess << std::endl;
			}
		}
		break;

	case 'x':
		EYE.x += 1.0f;
		View_Matrix = glm::lookAt(EYE, AT, UP);
		break;
	case 'X':
		EYE.x -= 1.0f;
		View_Matrix = glm::lookAt(EYE, AT, UP);
		break;
	case 'z':
		EYE.z += 1.0f;
		View_Matrix = glm::lookAt(EYE, AT, UP);
		break;
	case 'Z':
		EYE.z -= 1.0f;
		View_Matrix = glm::lookAt(EYE, AT, UP);
		break;
	case ' ':
		if (glutGetModifiers() == GLUT_ACTIVE_SHIFT) {
			EYE.y -= 1.0f;
		}
		else {
			EYE.y += 1.0f;
		}
		View_Matrix = glm::lookAt(EYE, AT, UP);
		break;
	case 'y':
		Camera_Rotation_Mode = (Camera_Rotation_Mode + 1) % 3;
		break;

	case 'r': {
		WaitingForMazeInput = true;

		drawScene();
		glFlush();

		int rows, cols;
		std::cout << "Range of rows and columns: 5 to 25\n";
		std::cout << "Enter number of rows and columns for the maze (e.g., 9 9): ";
		std::cin >> rows >> cols;
		if (rows <= 4 || cols <= 4) {
			rows = 5;
			cols = 5;
		}
		else if (rows > 25 || cols > 25) {
			rows = 25;
			cols = 25;
		}

		WaitingForMazeInput = false;

		g_Maze = MakeMaze(rows, cols);
		for (int i = 0; i < g_Maze.height; ++i) {
			for (int j = 0; j < g_Maze.width; ++j) {
				g_Maze.wallHeights[i][j] = 3.141592f;
				std::cout << (g_Maze.grid[i][j] == 1 ? "#" : " ");
			}
			std::cout << "\n";
		}
		CreatingWalls = true;
		RobotInWorld = true;
		ScaleWallHeight = true;
		MakeRobotAtMazeEntrance();

		std::cout << "Generated a new maze of size " << rows << "x" << cols << ".\n";
		break;
	}
	case '+':
		animation_speed_factor *= 1.2f;

		std::cout << "Increased animation speed factor: " << animation_speed_factor << "\n";

		break; 
	case '-':
		animation_speed_factor /= 1.2f;

		std::cout << "Decreased animation speed factor: " << animation_speed_factor << "\n";
		break;

	case 'o':
		Perspective_On = false;
		std::cout << "Orthographic Projection Mode\n";
		break;
	case 'p':
		Perspective_On = true;
		std::cout << "Perspective Projection Mode\n";
		break;

	case 'f':
		// create robot in world
		MakeRobotAtMazeEntrance();

		std::cout << "Robot In World at Maze Entrance" << ((RobotInWorld) ? "On\n" : "Off\n");
		break;
	case '1':
		RobotThirdPersonView = false;

		std::cout << "Robot First Person View Off\n";
		break;
	case '3':
		RobotThirdPersonView = true;

		std::cout << "Robot Third Person View On\n";
		break;
	case 'q':
		exit(0);
	default:
		break;
	}

	// Robot Movement Direction Setting (키 누름 시 방향만 설정)
	if (RobotInWorld) {
		switch (key) {
		case 'w':
		case 's':
		case 'a':
		case 'd': {
			// 키 상태를 기반으로 방향 벡터 재계산
			Robot_Direction = glm::vec3(0.0f);
			if (keyStates['w']) Robot_Direction.z += 1.0f;
			if (keyStates['s']) Robot_Direction.z -= 1.0f;
			if (keyStates['a']) Robot_Direction.x += 1.0f;
			if (keyStates['d']) Robot_Direction.x -= 1.0f;

			float dirLength = glm::length(Robot_Direction);
			if (dirLength > 0.01f) {
				Robot_Direction = glm::normalize(Robot_Direction);
			}
			break;
		}
		default:
			break;
		}
	}
}
void KeyBoardUp(unsigned char key, int x, int y) {
	keyStates[key] = false;

	if (key == 'w' || key == 's' || key == 'a' || key == 'd') {
		if (RobotInWorld) {
			// 방향 벡터 재계산
			Robot_Direction = glm::vec3(0.0f);
			if (keyStates['w']) Robot_Direction.z += 1.0f;
			if (keyStates['s']) Robot_Direction.z -= 1.0f;
			if (keyStates['a']) Robot_Direction.x += 1.0f;
			if (keyStates['d']) Robot_Direction.x -= 1.0f;

			float dirLength = glm::length(Robot_Direction);
			if (dirLength > 0.01f) {
				Robot_Direction = glm::normalize(Robot_Direction);
			}
			else {
				Robot_Direction = glm::vec3(0.0f);
			}
		}
		else {
			Robot_Direction = glm::vec3(0.0f);
		}
	}
}
void SpecialKeyBoard(int key, int x, int y) {
	specialKeyStates[key] = true;

}
void SpecialKeyBoardUp(int key, int x, int y) {
	specialKeyStates[key] = false;

}
void MouseClick(int button, int state, int x, int y) {
	// 마우스 휠 버튼 클릭으로 마우스 룩 모드 토글
	if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN && RobotInWorld) {
		MouseLookMode = !MouseLookMode;
		FirstMouse = true;

		if (MouseLookMode) {
			glutSetCursor(GLUT_CURSOR_NONE);
			
			int centerX = glutGet(GLUT_WINDOW_WIDTH) / 2;
			int centerY = glutGet(GLUT_WINDOW_HEIGHT) / 2;
			glutWarpPointer(centerX, centerY);
			std::cout << "Mouse Look Mode: ON (Middle Click)\n";
		}
		else {
			glutSetCursor(GLUT_CURSOR_INHERIT);
			std::cout << "Mouse Look Mode: OFF (Middle Click)\n";
		}
	}
}
void MouseMotion(int x, int y) {
	if (!MouseLookMode || !RobotInWorld) return;

	int width = glutGet(GLUT_WINDOW_WIDTH);
	int height = glutGet(GLUT_WINDOW_HEIGHT);
	int centerX = width / 2;
	int centerY = height / 2;

	// 첫 마우스 입력 무시
	if (FirstMouse) {
		LastMouseX = centerX;
		LastMouseY = centerY;
		FirstMouse = false;
		return;
	}

	int deltaX = x - centerX;
	int deltaY = y - centerY;

	if (abs(deltaX) < 2 && abs(deltaY) < 2) {
		return;
	}

	float yaw = -deltaX * MouseSensitivity;
	float pitch = -deltaY * MouseSensitivity;

	Robot_Rotation_Y += yaw;
	if (Robot_Rotation_Y > 360.0f) Robot_Rotation_Y -= 360.0f;
	if (Robot_Rotation_Y < 0.0f) Robot_Rotation_Y += 360.0f;

	Camera_Pitch += pitch;
	if (Camera_Pitch > 89.0f) Camera_Pitch = 89.0f;
	if (Camera_Pitch < -89.0f) Camera_Pitch = -89.0f;

	glutWarpPointer(centerX, centerY);
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
		"Figures.obj",
		"Wall.obj",
		"Robot.obj",
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
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_glm), (GLvoid*)offsetof(Vertex_glm, normal));
			glEnableVertexAttribArray(2);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.IBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, object.indices.size() * sizeof(unsigned int), object.indices.data(), GL_STATIC_DRAW);

			std::cout << "  L  Buffered Object: " << object.name << ", VAO: " << object.VAO << "\n";

			// 각 객체의 로컬 AABB 계산 및 저장
			g_LocalAABBs[object.name] = CalculateAABB(object.vertices);
			std::cout << "     Calculated AABB for " << object.name << "\n";
		}
	}
	glBindVertexArray(0);
		
	std::cout << "Setup Light Sources...\n";
	MakeLightSources();

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

	ComposeOribit();
}

void RenderText(const char* text, float x, float y, void* font) {
	glUseProgram(0); // 쉐이더 비활성화

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT));

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glColor3f(1.0f, 1.0f, 1.0f); // 흰색
	glRasterPos2f(x, y);

	for (const char* c = text; *c != '\0'; c++) {
		glutBitmapCharacter(font, *c);
	}

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glUseProgram(shaderProgramID); // 쉐이더 재활성화
}
void DisplayParameters(float deltaTime) {
	glDisable(GL_DEPTH_TEST);

	int height = glutGet(GLUT_WINDOW_HEIGHT);
	int width = glutGet(GLUT_WINDOW_WIDTH);
	float yPos = height - 20; // 상단에서 20픽셀 아래부터 시작
	float lineHeight = 15;    // 줄 간격

	char buffer[256];

	if (WaitingForMazeInput) {
		char inputPrompt[256];
		sprintf(inputPrompt, "Enter Maze Size in Console");

		float centerX = width / 2.0f - 120.0f;
		float centerY = height / 2.0f;

		glUseProgram(0);
		glDisable(GL_DEPTH_TEST);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		gluOrtho2D(0, width, 0, height);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
		glBegin(GL_QUADS);
		glVertex2f(centerX - 20, centerY - 30);
		glVertex2f(centerX + 290, centerY - 30);
		glVertex2f(centerX + 290, centerY + 30);
		glVertex2f(centerX - 20, centerY + 30);
		glEnd();
		glDisable(GL_BLEND);

		glColor3f(1.0f, 1.0f, 0.0f);
		glRasterPos2f(centerX, centerY);
		for (const char* c = inputPrompt; *c != '\0'; c++) {
			glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
		}

		glColor3f(1.0f, 1.0f, 1.0f);
		char subText[] = "(Check Console Window)";
		glRasterPos2f(centerX + 20, centerY - 20);
		for (const char* c = subText; *c != '\0'; c++) {
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
		}

		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);

		glEnable(GL_DEPTH_TEST);
		glUseProgram(shaderProgramID);

		return;
	}

	// FPS
	sprintf(buffer, "FPS: %.1f", 1.0f / deltaTime);
	RenderText(buffer, 10, yPos);
	yPos -= lineHeight;

	// Projection Mode
	sprintf(buffer, "Projection: %s", Perspective_On ? "Perspective" : "Orthographic");
	RenderText(buffer, 10, yPos);
	yPos -= lineHeight;

	// Light Revolution
	const char* revolutionState = (Revolution_Mode == 0) ? "Stopped" :
		(Revolution_Mode == 1) ? "+Y Axis" : "-Y Axis";
	sprintf(buffer, "Light Revolution: %s", revolutionState);
	RenderText(buffer, 10, yPos);
	yPos -= lineHeight;

	// Camera Rotation
	const char* cameraRotState = (Camera_Rotation_Mode == 0) ? "Stopped" :
		(Camera_Rotation_Mode == 1) ? "+Y Axis" : "-Y Axis";
	sprintf(buffer, "Camera Rotation: %s", cameraRotState);
	RenderText(buffer, 10, yPos);
	yPos -= lineHeight;

	// Wall Animation
	sprintf(buffer, "Wall Height Animation: %s", ScaleWallHeight ? "ON" : "OFF");
	RenderText(buffer, 10, yPos);
	yPos -= lineHeight;

	// Animation Speed Factor
	sprintf(buffer, "Animation Speed: %.2fx", animation_speed_factor);
	RenderText(buffer, 10, yPos);
	yPos -= lineHeight;

	// Maze Size
	sprintf(buffer, "Maze Size: %dx%d", g_Maze.width, g_Maze.height);
	RenderText(buffer, 10, yPos);
	yPos -= lineHeight;

	// Robot Info
	if (RobotInWorld) {
		yPos -= lineHeight * 0.5f;

		sprintf(buffer, "=== ROBOT INFO ===");
		RenderText(buffer, 10, yPos);
		yPos -= lineHeight;

		sprintf(buffer, "Position: (%.1f, %.1f, %.1f)",
			Robot_Position.x, Robot_Position.y, Robot_Position.z);
		RenderText(buffer, 10, yPos);
		yPos -= lineHeight;

		sprintf(buffer, "Rotation: %.1f deg", Robot_Rotation_Y);
		RenderText(buffer, 10, yPos);
		yPos -= lineHeight;

		sprintf(buffer, "View Mode: %s", RobotThirdPersonView ? "3rd Person" : "1st Person");
		RenderText(buffer, 10, yPos);
		yPos -= lineHeight;

		sprintf(buffer, "Speed: %.1f (Shift: %.1f)", Robot_Walk_Speed, Robot_Run_Speed);
		RenderText(buffer, 10, yPos);
		yPos -= lineHeight;

		sprintf(buffer, "Mouse Look: %s", MouseLookMode ? "ON" : "OFF");
		RenderText(buffer, 10, yPos);
		yPos -= lineHeight;
	}
	else {
		yPos -= lineHeight * 0.5f;
		sprintf(buffer, "Robot: Not in World (Press 'f')");
		RenderText(buffer, 10, yPos);
		yPos -= lineHeight;
	}

	glEnable(GL_DEPTH_TEST);
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

			unsigned int vertexIndex[4] = { 0,0,0,0 }, uvIndex[4] = { 0,0,0,0 }, normalIndex[4] = { 0,0,0,0 };
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d\n",
				&vertexIndex[0], &uvIndex[0], &normalIndex[0],
				&vertexIndex[1], &uvIndex[1], &normalIndex[1],
				&vertexIndex[2], &uvIndex[2], &normalIndex[2],
				&vertexIndex[3], &uvIndex[3], &normalIndex[3]);

			auto pushVertex = [&](int vi, int ni) {
				Vertex_glm vertex;
				vertex.position = temp_vertices[vi - 1];
				vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
				if (!temp_normals.empty() && ni > 0) {
					vertex.normal = temp_normals[ni - 1];
				}
				else {
					// 법선 정보가 없다면 임시로 (0,1,0) 넣어두고 이후 셰이더에서 보정 가능
					vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
				}
				currentObject->vertices.push_back(vertex);
				currentObject->indices.push_back(currentObject->vertices.size() - 1);
				};

			if (matches == 9) { // 삼각형 (3개: v/vt/vn)
				pushVertex(vertexIndex[0], normalIndex[0]);
				pushVertex(vertexIndex[1], normalIndex[1]);
				pushVertex(vertexIndex[2], normalIndex[2]);
			}
			else if (matches == 12) { // 사각형 -> 삼각형 2개로 분할
				// 0, 1, 2
				pushVertex(vertexIndex[0], normalIndex[0]);
				pushVertex(vertexIndex[1], normalIndex[1]);
				pushVertex(vertexIndex[2], normalIndex[2]);
				// 0, 2, 3
				pushVertex(vertexIndex[0], normalIndex[0]);
				pushVertex(vertexIndex[2], normalIndex[2]);
				pushVertex(vertexIndex[3], normalIndex[3]);
			}
			else {
				// 다른 형식의 면 데이터는 현재 파서에서 지원하지 않음
				char buffer[1024];
				fgets(buffer, 1024, file);
			}

			// calculate origin
			glm::vec3 origin(0.0f);
			for (const auto& vertex : currentObject->vertices) {
				origin += vertex.position;
			}
			if (!currentObject->vertices.empty()) {
				origin /= static_cast<float>(currentObject->vertices.size());
				currentObject->origin = origin;
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
void MakeDynamicMatrix(float deltaTime) {
	// Robot Direction Reset if no movement keys are pressed
	// 가끔 이동	키를 모두 떼었을 때 방향 벡터가 남아있는 경우 방지
	bool anyKeyPressed = keyStates['w'] || keyStates['s'] ||
		keyStates['a'] || keyStates['d'];

	if (!RobotInWorld || !anyKeyPressed) {
		Robot_Direction = glm::vec3(0.0f);
	}

	// Robot Movement
	if (RobotInWorld && glm::length(Robot_Direction) > 0.01f) {
		float rad = glm::radians(Robot_Rotation_Y);
		glm::mat3 rotationMatrix = glm::mat3(
			cos(-rad), 0.0f, sin(-rad),
			0.0f, 1.0f, 0.0f,
			-sin(-rad), 0.0f, cos(-rad)
		);

		if (specialKeyStates[GLUT_KEY_SHIFT_L]) {
			Robot_Speed = Robot_Run_Speed;
		}
		else {
			Robot_Speed = Robot_Walk_Speed;
		}

		glm::vec3 worldDirection = rotationMatrix * Robot_Direction;
		glm::vec3 newPosition = Robot_Position + worldDirection * Robot_Speed * deltaTime;

		if (!CheckRobotWallCollision(newPosition)) {
			Robot_Position = newPosition;
		}
	}

	Robot_Matrix = glm::mat4(1.0f);
	Robot_Matrix = glm::translate(Robot_Matrix, Robot_Position);
	Robot_Matrix = glm::rotate(Robot_Matrix, glm::radians(Robot_Rotation_Y), glm::vec3(0.0f, 1.0f, 0.0f));
	Robot_Matrix = glm::scale(Robot_Matrix, Robot_Scale);

	// Camera Setup
	if (RobotInWorld) {
		float scale_factor = (Robot_Scale.x + Robot_Scale.y + Robot_Scale.z) / 3.0f;
		float yaw_rad = glm::radians(Robot_Rotation_Y);
		float pitch_rad = glm::radians(Camera_Pitch);

		// 카메라 시야 방향 계산
		glm::vec3 cameraDirection;
		cameraDirection.x = cos(pitch_rad) * sin(yaw_rad);
		cameraDirection.y = sin(pitch_rad);
		cameraDirection.z = cos(pitch_rad) * cos(yaw_rad);
		cameraDirection = glm::normalize(cameraDirection);

		if (RobotThirdPersonView) {
			// 3인칭 시점
			glm::vec3 localOffset = glm::vec3(-1.5f, 12.0f, -5.0f) * scale_factor;

			glm::mat3 yawRotationMatrix = glm::mat3(
				cos(-yaw_rad), 0.0f, sin(-yaw_rad),
				0.0f, 1.0f, 0.0f,
				-sin(-yaw_rad), 0.0f, cos(-yaw_rad)
			);
			glm::vec3 worldOffset = yawRotationMatrix * localOffset;
			EYE = Robot_Position + worldOffset;

			AT = Robot_Position + cameraDirection * (50.0f * scale_factor) + glm::vec3(0.0f, 2.0f * scale_factor, 0.0f);
		}
		else {
			// 1인칭 시점
			glm::vec3 localOffset = glm::vec3(0.0f, 10.0f, 0.5f) * scale_factor;

			glm::mat3 yawRotationMatrix = glm::mat3(
				cos(-yaw_rad), 0.0f, sin(-yaw_rad),
				0.0f, 1.0f, 0.0f,
				-sin(-yaw_rad), 0.0f, cos(-yaw_rad)
			);
			glm::vec3 worldOffset = yawRotationMatrix * localOffset;
			EYE = Robot_Position + worldOffset;
			AT = EYE + cameraDirection * (50.0f * scale_factor);
		}
	}
	else {
		glm::vec4 EYE_vec4 = glm::vec4(EYE, 1.0f);
		if (Camera_Rotation_Mode == 1) {
			glm::mat4 rotationMat = glm::mat4(1.0f);
			rotationMat = glm::rotate(rotationMat, glm::radians(Camera_Rotation_Factor.y * deltaTime), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::vec4 rotatedEYE = rotationMat * EYE_vec4;
			EYE = glm::vec3(rotatedEYE);
		}
		else if (Camera_Rotation_Mode == 2) {
			glm::mat4 rotationMat = glm::mat4(1.0f);
			rotationMat = glm::rotate(rotationMat, glm::radians(-Camera_Rotation_Factor.y * deltaTime), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::vec4 rotatedEYE = rotationMat * EYE_vec4;
			EYE = glm::vec3(rotatedEYE);
		}
	}

	View_Matrix = glm::lookAt(EYE, AT, UP);


	Model_Scale = glm::vec3(1.0f, 1.0f, 1.0f);

	Model_Matrix = glm::mat4(1.0f);
	Model_Matrix = glm::scale(Model_Matrix, Model_Scale);
	Model_Matrix = glm::translate(Model_Matrix, Model_Transform);

	if (Rotation_Mode == 1) {
		Floor_Rotation_Angle += Floor_Rotation_Factor * deltaTime;
		Tank_Rotation_Angle += Tank_Rotation_Factor * deltaTime;
	}
	else if (Rotation_Mode == 2) {
		Floor_Rotation_Angle -= Floor_Rotation_Factor * deltaTime;
		Tank_Rotation_Angle -= Tank_Rotation_Factor * deltaTime;
	}

	if (Revolution_Mode == 1) {
		Light_Revolution_Angle += Light_Revolution_Factor * deltaTime;
	}
	else if (Revolution_Mode == 2) {
		Light_Revolution_Angle -= Light_Revolution_Factor * deltaTime;
	}

	Floor_Matrix = glm::mat4(1.0f);
	glm::vec3 floorScale = glm::vec3(g_Maze.width, 1.0f, g_Maze.height);
	floorScale *= 0.5f;
	Floor_Matrix = glm::scale(Floor_Matrix, floorScale);

	// Light Revolution
	for (auto& light : g_Lights) {
		glm::vec4 initPos = glm::vec4(light.init_position, 1.0f);
		glm::mat4 revolutionMat = glm::mat4(1.0f);
		revolutionMat = glm::rotate(revolutionMat, glm::radians(Light_Revolution_Angle), glm::vec3(0.0f, 1.0f, 0.0f));
		revolutionMat = glm::translate(revolutionMat, Light_Trasform);
		glm::vec4 rotatedPos = revolutionMat * initPos;
		light.light_vertex.position = glm::vec3(rotatedPos);
	
		if (light.VBO != 0) {
			glBindBuffer(GL_ARRAY_BUFFER, light.VBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex_glm), &light.light_vertex);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
	}
}

void GetUniformLocations() {
	// static uniform variable
	PerspectiveMatrixID = glGetUniformLocation(shaderProgramID, "Perspective_Matrix");
	ViewMatrixID = glGetUniformLocation(shaderProgramID, "View_Matrix");
	ModelMatrixID = glGetUniformLocation(shaderProgramID, "Model_Matrix");
	FloorMatrixID = glGetUniformLocation(shaderProgramID, "Floor_Matrix");
	TankMatrixID = glGetUniformLocation(shaderProgramID, "Tank_Matrix");
	ViewPosID = glGetUniformLocation(shaderProgramID, "viewPos");
	ShininessID = glGetUniformLocation(shaderProgramID, "shininess");
	RobotMatrixID = glGetUniformLocation(shaderProgramID, "Robot_Matrix");

	// dynamic uniform variable
	FigureTypeID = glGetUniformLocation(shaderProgramID, "Figure_Type");
	WallMatrixID = glGetUniformLocation(shaderProgramID, "Wall_Matrix");
}
void UpdateUniformMatrices() {
	glUniformMatrix4fv(PerspectiveMatrixID, 1, GL_FALSE, &Perspective_Matrix[0][0]);
	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &View_Matrix[0][0]);
	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Model_Matrix[0][0]);
	glUniformMatrix4fv(FloorMatrixID, 1, GL_FALSE, &Floor_Matrix[0][0]);
	glUniformMatrix4fv(TankMatrixID, 1, GL_FALSE, &Tank_Matrix[0][0]);
	glUniform3fv(ViewPosID, 1, &EYE[0]);
	glUniformMatrix4fv(RobotMatrixID, 1, GL_FALSE, &Robot_Matrix[0][0]);

	if (PerspectiveMatrixID == -1) std::cerr << "Could not bind uniform Perspective_Matrix\n";
	if (ViewMatrixID == -1) std::cerr << "Could not bind uniform View_Matrix\n";
	if (ModelMatrixID == -1) std::cerr << "Could not bind uniform Model_Matrix\n";
	if (FloorMatrixID == -1) std::cerr << "Could not bind uniform Cube_Matrix\n";
	if (TankMatrixID == -1) std::cerr << "Could not bind uniform Pyramid_Matrix\n";
	if (ViewPosID == -1) std::cerr << "Could not bind uniform viewPos\n";
	if (RobotMatrixID == -1) std::cerr << "Could not bind uniform Robot_Matrix\n";

}
void ComposeOBJColor() {
	for (auto& file : g_OBJ_Files) {
		for (auto& object : file.objects) {
			if (object.name == "Floor") {
				glm::vec3 bodyColor(0.5f, 0.8f, 0.5f);
				for (auto& vertex : object.vertices) {
					vertex.color = bodyColor;
				}
			}
			else if (object.name == "Tank") {
				glm::vec3 roofColor(0.5f, 0.8f, 0.5f);
				for (auto& vertex : object.vertices) {
					vertex.color = roofColor;
				}
				object.shininess = 128.0f;
			}
			else if (object.name == "Wall") {
				glm::vec3 brickColor(0.72f, 0.37f, 0.28f);
				for (auto& vertex : object.vertices) {
					vertex.color = brickColor;
				}
				object.shininess = 1.0f;
			}
		}
	}
}
void ComposeOribit() {
	// Create Orbit Path
	std::cout << "Setup Orbit...\n";
	orbit_vertices.clear();
	orbit_indices.clear();

	const int orbit_segments = 100;
	const glm::vec3 orbit_y = glm::vec3(0.0, Light_Trasform.y, 0.0);
	const glm::vec3 orbit_center_for_radius = Light_Trasform - orbit_y;
	const float orbit_radius = glm::length(orbit_center_for_radius);
	for (int i = 0; i <= orbit_segments; ++i) {
		float angle = 2.0f * glm::pi<float>() * static_cast<float>(i) / orbit_segments;
		Vertex_glm vertex;
		vertex.position.x = orbit_radius * cos(angle);
		vertex.position.y = Light_Trasform.y;
		vertex.position.z = orbit_radius * sin(angle);
		vertex.color = glm::vec3(0.8f, 0.8f, 0.2f);
		orbit_vertices.push_back(vertex);
		orbit_indices.push_back(i);
	}

	glGenVertexArrays(1, &VAO_orbit);
	glGenBuffers(1, &VBO_orbit);
	glGenBuffers(1, &IBO_orbit);

	glBindVertexArray(VAO_orbit);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_orbit);
	glBufferData(GL_ARRAY_BUFFER, orbit_vertices.size() * sizeof(Vertex_glm), orbit_vertices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_glm), (GLvoid*)offsetof(Vertex_glm, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_glm), (GLvoid*)offsetof(Vertex_glm, color));
	glEnableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO_orbit);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, orbit_indices.size() * sizeof(unsigned int), orbit_indices.data(), GL_STATIC_DRAW);


	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


}
void MakeLightSources() {
	// Create Light Source Buffers
	Light light1(glm::vec3(20.0f, 15.0f, 0.0f));
	light1.intensity = 5.0f;
	g_Lights.push_back(light1);

	Light light2(glm::vec3(-20.0f, 15.0f, 0.0f));
	light2.intensity = 5.0f;
	g_Lights.push_back(light2);

	Light light3(glm::vec3(0.0f, 15.0f, 20.0f));
	light3.intensity = 5.0f;
	g_Lights.push_back(light3);

	Light light4(glm::vec3(0.0f, 15.0f, -20.0f));
	light4.intensity = 5.0f;
	g_Lights.push_back(light4);

	for (auto& light : g_Lights) {
		glGenVertexArrays(1, &light.VAO);
		glGenBuffers(1, &light.VBO);
		glGenBuffers(1, &light.IBO);

		glBindVertexArray(light.VAO);

		glBindBuffer(GL_ARRAY_BUFFER, light.VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex_glm), &light.light_vertex, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_glm), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_glm), (GLvoid*)offsetof(Vertex_glm, color));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_glm), (GLvoid*)offsetof(Vertex_glm, normal));
		glEnableVertexAttribArray(2);

		unsigned int lightIndex = 0;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, light.IBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int), &lightIndex, GL_STATIC_DRAW);

		glBindVertexArray(0);
	}

}

void Type_distinction(const std::string& object_name, GLuint& outTypeID) {
	if (object_name == "Floor") {
		outTypeID = Figure_Type::FLOOR;
	}
	else if (object_name == "Tank") {
		outTypeID = Figure_Type::TANK;
	}
	else if (object_name == "Wall") {
		outTypeID = Figure_Type::WALL;
	}
	else if (object_name == "body" ||
		object_name == "right_arm" ||
		object_name == "left_arm" ||
		object_name == "right_leg" ||
		object_name == "left_leg") {
		outTypeID = Figure_Type::ROBOT;
	}
	else {
		outTypeID = Figure_Type::ETC;
	}
}

Maze MakeMaze(int N, int M) {
	// Eller의 알고리즘을 사용한 미로 생성
	if (N <= 0) N = 1;
	if (M <= 0) M = 1;
	Maze maze(M, N);
	std::vector<int> sets(M, 0);
	int next_set = 1;

	std::uniform_real_distribution<float> urd(0.0f, 1.0f);

	for (int i = 0; i < N; ++i) {
		// 1. 아직 집합에 속하지 않은 셀에 새 집합 할당
		for (int j = 0; j < M; ++j) {
			if (sets[j] == 0) {
				sets[j] = next_set++;
			}
		}

		// 2. 오른쪽 벽 생성
		for (int j = 0; j < M - 1; ++j) {
			bool create_wall = true;
			if (sets[j] != sets[j + 1]) {
				if (i == N - 1 || urd(dre) > 0.5f) {
					create_wall = false;
				}
			}

			if (!create_wall) {
				int old_set = sets[j + 1];
				for (int k = 0; k < M; ++k) {
					if (sets[k] == old_set) {
						sets[k] = sets[j];
					}
				}
				maze.grid[2 * i + 1][2 * (j + 1)] = 0;
			}
		}

		// 3. 아래쪽 벽 생성
		if (i < N - 1) {
			std::map<int, std::vector<int>> set_members;
			for (int j = 0; j < M; ++j) {
				set_members[sets[j]].push_back(j);
			}

			for (auto const& [set_id, members] : set_members) {
				int passages = 0;
				std::vector<int> shuffled_members = members;
				std::shuffle(shuffled_members.begin(), shuffled_members.end(), dre);

				for (int member_idx : shuffled_members) {
					if (passages < members.size() - 1 && urd(dre) > 0.5f) {
						
					}
					else {
						maze.grid[2 * (i + 1)][2 * member_idx + 1] = 0;
						passages++;
					}
				}
				if (passages == 0) {
					int rand_member_idx = shuffled_members[0];
					maze.grid[2 * (i + 1)][2 * rand_member_idx + 1] = 0;
				}
			}
		}

		// 4. 다음 행 준비
		for (int j = 0; j < M; ++j) {
			maze.grid[2 * i + 1][2 * j + 1] = 0;
			if (i < N - 1 && maze.grid[2 * (i + 1)][2 * j + 1] == 1) {
				sets[j] = 0;
			}
		}
	}

	maze.grid[maze.startY][maze.startX] = 0;
	maze.grid[maze.enterPointY][maze.enterPointX] = 0;

	maze.exitPointX = 2 * (M - 1) + 1;
	maze.exitPointY = 2 * N;
	maze.grid[maze.exitPointY][maze.exitPointX] = 0;

	return maze;
}
void MakeRobotAtMazeEntrance() {
	RobotInWorld = !RobotInWorld;
	if (!RobotInWorld) {
		EYE = glm::vec3(0.0f, 30.0f, 50.0f);
		AT = glm::vec3(0.0f, 10.0f, 0.0f);

		Robot_Position = glm::vec3(0.0f);
		Robot_Rotation_Y = 0.0f;
		Robot_Direction = glm::vec3(0.0f);
		Camera_Pitch = 0.0f;
		MouseLookMode = false;

		keyStates['w'] = false;
		keyStates['s'] = false;
		keyStates['a'] = false;
		keyStates['d'] = false;

		glutSetCursor(GLUT_CURSOR_INHERIT);

		return;
	}

	AABB wallAABB = g_LocalAABBs.at("Wall");
	glm::vec3 wallSize = wallAABB.max - wallAABB.min;

	float x_offset = -g_Maze.width * wallSize.x / 2.0f;
	float z_offset = -g_Maze.height * wallSize.z / 2.0f;

	float entrance_world_x = g_Maze.enterPointX * wallSize.x + x_offset;
	float entrance_world_z = g_Maze.enterPointY * wallSize.z + z_offset;

	std::cout << "Entrance Position (Grid): (" << g_Maze.enterPointX << ", " << g_Maze.enterPointY << ")\n";
	std::cout << "Entrance Position (World): (" << entrance_world_x << ", " << entrance_world_z << ")\n";

	if (RobotInWorld) {
		Robot_Position = glm::vec3(entrance_world_x, 0.0f, entrance_world_z);
		//Robot_Scale = glm::vec3(0.5f);
		Robot_Rotation_Y = 0.0f;

		std::cout << "Robot spawned at maze entrance\n";
	}
	else {
		Robot_Position = glm::vec3(0.0f);
		//Robot_Scale = glm::vec3(0.5f);
		Robot_Rotation_Y = 0.0f;

		std::cout << "Robot removed from world\n";
	}
}

bool CheckRobotWallCollision(const glm::vec3& newPosition) {
	AABB robotLocalAABB = CalculateRobotAABB();

	glm::mat4 robotTestMatrix = glm::mat4(1.0f);
	robotTestMatrix = glm::translate(robotTestMatrix, newPosition);
	robotTestMatrix = glm::rotate(robotTestMatrix, glm::radians(Robot_Rotation_Y), glm::vec3(0.0f, 1.0f, 0.0f));
	robotTestMatrix = glm::scale(robotTestMatrix, Robot_Scale);

	AABB robotWorldAABB = TransformAABB(robotLocalAABB, robotTestMatrix);

	AABB wallLocalAABB = g_LocalAABBs.at("Wall");
	glm::vec3 wallSize = wallLocalAABB.max - wallLocalAABB.min;

	// 로봇 주변 그리드만 체크
	float halfWidth = (robotWorldAABB.max.x - robotWorldAABB.min.x) / 2.0f;
	float halfDepth = (robotWorldAABB.max.z - robotWorldAABB.min.z) / 2.0f;

	int gridX = (int)((newPosition.x + g_Maze.width * wallSize.x / 2.0f) / wallSize.x);
	int gridZ = (int)((newPosition.z + g_Maze.height * wallSize.z / 2.0f) / wallSize.z);

	// 주변 3x3 그리드만 체크
	for (int i = gridZ - 1; i <= gridZ + 1; ++i) {
		for (int j = gridX - 1; j <= gridX + 1; ++j) {
			if (i >= 0 && i < g_Maze.height && j >= 0 && j < g_Maze.width) {
				if (g_Maze.grid[i][j] == 1) {
					// 벽의 변환 행렬 계산
					float x_offset = -g_Maze.width * wallSize.x / 2.0f;
					float z_offset = -g_Maze.height * wallSize.z / 2.0f;

					glm::vec3 wallPosition(j * wallSize.x + x_offset, 0.0f, i * wallSize.z + z_offset);

					glm::mat4 wallMatrix = glm::mat4(1.0f);
					wallMatrix = glm::translate(wallMatrix, wallPosition);

					// 벽의 월드 AABB 계산
					AABB wallWorldAABB = TransformAABB(wallLocalAABB, wallMatrix);

					// 충돌 체크 (XZ 평면만 체크, Y축은 무시)
					bool collisionX = (robotWorldAABB.min.x <= wallWorldAABB.max.x && robotWorldAABB.max.x >= wallWorldAABB.min.x);
					bool collisionZ = (robotWorldAABB.min.z <= wallWorldAABB.max.z && robotWorldAABB.max.z >= wallWorldAABB.min.z);

					if (collisionX && collisionZ) {
						return true;
					}
				}
			}
		}
	}

	return false;
}
AABB CalculateRobotAABB() {
	std::vector<std::string> robotParts = {
		"body",
		"right_arm",
		"left_arm",
		"right_leg",
		"left_leg"
	};

	bool first = true;
	AABB combinedAABB;

	for (const auto& partName : robotParts) {
		auto it = g_LocalAABBs.find(partName);
		if (it != g_LocalAABBs.end()) {
			const AABB& partAABB = it->second;

			if (first) {
				combinedAABB = partAABB;
				first = false;
			}
			else {
				// 최소값 갱신
				combinedAABB.min.x = std::min(combinedAABB.min.x, partAABB.min.x);
				combinedAABB.min.y = std::min(combinedAABB.min.y, partAABB.min.y);
				combinedAABB.min.z = std::min(combinedAABB.min.z, partAABB.min.z);

				// 최대값 갱신
				combinedAABB.max.x = std::max(combinedAABB.max.x, partAABB.max.x);
				combinedAABB.max.y = std::max(combinedAABB.max.y, partAABB.max.y);
				combinedAABB.max.z = std::max(combinedAABB.max.z, partAABB.max.z);
			}
		}
	}

	return combinedAABB;
}
