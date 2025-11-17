#define _CRT_SECURE_NO_WARNINGS
#include "VarANDFunc_03_Maze.h"

auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
std::default_random_engine dre(seed);
std::uniform_real_distribution<float> urd_0_1(0.0f, 1.0f);
std::uniform_int_distribution<int> uid_0_3(0, 3);

glm::mat4 Perspective_Matrix(1.0f), View_Matrix(1.0f);
glm::mat4 Model_Matrix(1.0f);
glm::mat4 Floor_Matrix(1.0f), Tank_Matrix(1.0f);

std::vector<OBJ_File> g_OBJ_Files;
std::map<std::string, AABB> g_LocalAABBs; // 객체별 로컬 AABB 저장

GLuint VAO_orbit, VBO_orbit, IBO_orbit;;
std::vector<Light> g_Lights;
std::vector<Vertex_glm> orbit_vertices;
std::vector<unsigned int> orbit_indices;

Maze g_Maze(1, 1);

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowPosition(50, 50);
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
	Perspective_Matrix = glm::perspective(FOV, AspectRatio, NearClip, FarClip);
	//Perspective_Matrix = glm::ortho(-50.0f, 50.0f, -50.0f * (float)height / (float)width, 50.0f * (float)height / (float)width, NearClip, FarClip);

	// 2. Update Uniform Matrices
	UpdateUniformMatrices();

	// 3. Draw Models
	DrawModels(deltaTime);

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
			glUniform1i(FigureTypeID, Figure_Type::LIGHT); // LIGHT 타입 설정
			glPointSize(5.0f);
			glDrawElements(GL_POINTS, 1, GL_UNSIGNED_INT, 0); // 각 광원을 루프 안에서 그림
		}
	}
	else {
		glUniform1i(glGetUniformLocation(shaderProgramID, "numLights"), 0);
	}
	//glUniform1f(ShininessID, g_shininess);

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

		for (int i = 0; i < g_Maze.height; ++i) {
			for (int j = 0; j < g_Maze.width; ++j) {
				if (g_Maze.grid[i][j] == 1) {
					glm::mat4 wallModelMatrix = glm::mat4(1.0f);
					// 미로의 중앙을 (0,0,0)으로 맞추기 위해 오프셋 계산
					float x_offset = -g_Maze.width * wallSize.x / 2.0f;
					float z_offset = -g_Maze.height * wallSize.z / 2.0f;

					float height_scale = WallMaxHeight;
					if (ScaleWallHeight) {
						const float pi = 3.141592f;
						const float animation_speed = 2.0f * urd_0_1(dre); // 애니메이션 속도 조절
						float& theta = g_Maze.wallHeights[i][j]; // 각 벽의 현재 각도(theta)

						// theta 값을 시간에 따라 계속 증가시킴
						theta += animation_speed * deltaTime;
						if (theta > 2.0f * pi) {
							theta -= 2.0f * pi;
						}

						// cos(theta)를 사용하여 -1 ~ 1 범위의 값을 얻고, 이를 0 ~ 1 범위로 변환
						float cos_value = cos(theta);
						float normalized_height = (cos_value + 1.0f) * 0.5f; // 0.0 ~ 1.0 범위

						// 최종 높이를 1.0(기본 크기) ~ WallMaxHeight 사이로 매핑
						height_scale = 1.0f + (WallMaxHeight - 1.0f) * normalized_height;
					}

					glm::vec3 scale = glm::vec3(1.0f, height_scale, 1.0f);
					glm::vec3 position(j * wallSize.x + x_offset, 0.0f, i * wallSize.z + z_offset);
					wallModelMatrix = glm::scale(wallModelMatrix, scale);
					wallModelMatrix = glm::translate(wallModelMatrix, position);



					glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &wallModelMatrix[0][0]);
					glDrawElements(GL_TRIANGLES, wallObject->indices.size(), GL_UNSIGNED_INT, 0);
					//std::cout << "Create Wall at : ( " << i << ", " << j << " )" << '\n';
				}
				else {
					continue;
				}
			}
		}
		// 원래 ModelMatrix로 복원
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Model_Matrix[0][0]);
	}

	// Draw OBJ Models
	for (const auto& file : g_OBJ_Files) {
		for (const auto& object : file.objects) {
			if (!object.name.find("Tank") ||
				!object.name.find("Wall") ) {
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
	case 'm':
		ScaleWallHeight = !ScaleWallHeight;
		if (ScaleWallHeight) {
			for (int i = 0; i < g_Maze.height; ++i) {
				for (int j = 0; j < g_Maze.width; ++j) {
					g_Maze.wallHeights[i][j] = glm::max(urd_0_1(dre) * WallMaxHeight, 1.0f);
				}
			}
		}
		else {
			for (int i = 0; i < g_Maze.height; ++i) {
				for (int j = 0; j < g_Maze.width; ++j) {
					g_Maze.wallHeights[i][j] = WallMaxHeight;
				}
			}

		}

		std::cout << "Wall Height Scaling: " << (ScaleWallHeight ? "ON\n" : "OFF\n");
		break;
	case 'M':

		break;
	case 'j':
		Revolution_Mode = (Revolution_Mode + 1) % 3;

		if (Revolution_Mode == 0)
			std::cout << "Revolution Stopped\n";
		else if (Revolution_Mode == 1)
			std::cout << "Revolution +Y Axis\n";
		else if (Revolution_Mode == 2)
			std::cout << "Revolution -Y Axis\n";
		break;
	case 's':
		Revolution_Mode = 0;
		Camera_Rotation_Mode = 0;

		std::cout << "All Rotation Stopped\n";
		break;
	case 'l':
		Light_Trasform.z += 0.5f;
		Light_Trasform.z = std::max(Light_Trasform.z, 0.0f);

		ComposeOribit();
		break;
	case 'L':
		Light_Trasform.z -= 0.5f;
		Light_Trasform.z = std::max(Light_Trasform.z, 0.0f);

		ComposeOribit();
		break;
	case 'c':
		light_color_template_index = (light_color_template_index + 1) % light_color_template.size();
		
		for (auto& light : g_Lights) {
			light.light_color = light_color_template[light_color_template_index];
			light.light_vertex.color = light.light_color;
		}
		
		break;

	case 'k':
		// 이제 키보드로는 특정 객체의 shininess를 조절하거나,
		// 모든 객체를 순회하며 변경해야 합니다.
		for (auto& file : g_OBJ_Files) {
			for (auto& object : file.objects) {
				if (object.name == "Tank") { // 예: Tank만 조절
					object.shininess *= 2.0f;
					std::cout << "Tank Shininess: " << object.shininess << std::endl;
				}
			}
		}
		break;
	case 'K':
		for (auto& file : g_OBJ_Files) {
			for (auto& object : file.objects) {
				if (object.name == "Tank") {
					object.shininess /= 2.0f;
					if (object.shininess < 1.0f) object.shininess = 1.0f;
					std::cout << "Tank Shininess: " << object.shininess << std::endl;
				}
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
			// SHIFT + SPACEBAR: 카메라 아래로 이동
			EYE.y -= 1.0f;
		}
		else {
			// SPACEBAR: 카메라 위로 이동
			EYE.y += 1.0f;
		}
		View_Matrix = glm::lookAt(EYE, AT, UP);
		break;
	case 'y':
		Camera_Rotation_Mode = (Camera_Rotation_Mode + 1) % 3;

		break;

	case 'r':
		// make a new maze
		int rows, cols;
		std::cout << "Enter number of rows and columns for the maze (e.g., 5 5): ";
		std::cin >> rows >> cols;

		g_Maze = MakeMaze(rows, cols);
		for (int i = 0; i < g_Maze.height; ++i) {
			for (int j = 0; j < g_Maze.width; ++j) {
				std::cout << (g_Maze.grid[i][j] == 1 ? "#" : " ");
			}
			std::cout << "\n";
		}
		std::cout << "Generated a new maze of size " << rows << "x" << cols << ".\n";
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
		"Figures.obj",
		"Wall.obj",
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

	// Create Light Source Buffers
	Light light1(glm::vec3(0.0f, 5.5f, 0.0f));
	light1.intensity = 5.0f;
	g_Lights.push_back(light1);

	Light light2(glm::vec3(20.0f, 5.5f, -20.0f));
	light2.intensity = 5.0f;
	g_Lights.push_back(light2);

	Light light3(glm::vec3(-20.0f, 5.5f, -20.0f));
	light3.intensity = 5.0f;
	g_Lights.push_back(light3);

	Light light4(glm::vec3(0.0f, 5.5f, -40.0f));
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
				fgets(buffer, 1024, file); // 해당 라인의 나머지를 읽고 무시
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
	// Camera Rotation
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

	View_Matrix = glm::lookAt(EYE, AT, UP);


	Model_Scale = glm::vec3(5.0f, 5.0f, 5.0f);

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

	Custom_OBJ* cubeObject = nullptr;
	Custom_OBJ* pyramidObject = nullptr;
	for (auto& file : g_OBJ_Files) {
		for (auto& object : file.objects) {
			if (object.name == "Cube") {
				cubeObject = &object;
			}
			else if (object.name == "Pyramid") {
				pyramidObject = &object;
			}
		}
	}

	Floor_Matrix = glm::mat4(1.0f);
	if (cubeObject) {
		glm::mat4 local = glm::mat4(1.0f);
		local = glm::translate(local, cubeObject->origin);
		local = glm::rotate(local, glm::radians(Floor_Rotation_Angle), glm::vec3(0.0f, 1.0f, 0.0f));
		local = glm::translate(local, -cubeObject->origin);
		Floor_Matrix = local;
	}

	Tank_Matrix = glm::mat4(1.0f);
	if (pyramidObject) {
		glm::mat4 local = glm::mat4(1.0f);
		local = glm::translate(local, pyramidObject->origin);
		local = glm::rotate(local, glm::radians(Tank_Rotation_Angle), glm::vec3(0.0f, 1.0f, 0.0f));
		local = glm::translate(local, -pyramidObject->origin);
		Tank_Matrix = local;
	}

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

	// dynamic uniform variable
	FigureTypeID = glGetUniformLocation(shaderProgramID, "Figure_Type");
}
void UpdateUniformMatrices() {
	glUniformMatrix4fv(PerspectiveMatrixID, 1, GL_FALSE, &Perspective_Matrix[0][0]);
	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &View_Matrix[0][0]);
	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Model_Matrix[0][0]);
	glUniformMatrix4fv(FloorMatrixID, 1, GL_FALSE, &Floor_Matrix[0][0]);
	glUniformMatrix4fv(TankMatrixID, 1, GL_FALSE, &Tank_Matrix[0][0]);
	glUniform3fv(ViewPosID, 1, &EYE[0]);

	if (PerspectiveMatrixID == -1) std::cerr << "Could not bind uniform Perspective_Matrix\n";
	if (ViewMatrixID == -1) std::cerr << "Could not bind uniform View_Matrix\n";
	if (ModelMatrixID == -1) std::cerr << "Could not bind uniform Model_Matrix\n";
	if (FloorMatrixID == -1) std::cerr << "Could not bind uniform Cube_Matrix\n";
	if (TankMatrixID == -1) std::cerr << "Could not bind uniform Pyramid_Matrix\n";
	if (ViewPosID == -1) std::cerr << "Could not bind uniform viewPos\n";

}
void ComposeOBJColor() {
	for (auto& file : g_OBJ_Files) {
		for (auto& object : file.objects) {
			if (object.name == "Floor") {
				glm::vec3 bodyColor(0.8f, 0.5f, 0.5f);
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
	else {
		outTypeID = Figure_Type::ETC;
	}
}

Maze MakeMaze(int N, int M) {
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
				maze.grid[2 * i + 1][2 * (j + 1)] = 0; // 통로
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
						// 벽 유지
					}
					else {
						maze.grid[2 * (i + 1)][2 * member_idx + 1] = 0; // 통로
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
			maze.grid[2 * i + 1][2 * j + 1] = 0; // 셀 위치는 항상 통로
			if (i < N - 1 && maze.grid[2 * (i + 1)][2 * j + 1] == 1) {
				sets[j] = 0;
			}
		}
	}

	// 시작점 설정
	maze.grid[maze.startY][maze.startX] = 0;
	// 입구 설정
	maze.grid[maze.enterPointY][maze.enterPointX] = 0;

	// 출구 설정: 마지막 셀 아래에 출구 생성
	maze.exitPointX = 2 * (M - 1) + 1;
	maze.exitPointY = 2 * N;
	maze.grid[maze.exitPointY][maze.exitPointX] = 0;


	return maze;
}
