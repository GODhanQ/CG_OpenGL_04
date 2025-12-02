#define _CRT_SECURE_NO_WARNINGS
#include "VarANDFunc_Shader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
std::default_random_engine dre(seed);
std::uniform_real_distribution<float> urd_0_1(0.0f, 1.0f);
std::uniform_int_distribution<int> uid_0_3(0, 3);

glm::mat4 Perspective_Matrix(1.0f), View_Matrix(1.0f);
glm::mat4 Model_Matrix(1.0f);

glm::mat4 Cube_Matrix(1.0f), Cone_Matrix(1.0f);

std::vector<OBJ_File> g_OBJ_Files;
std::map<std::string, AABB> g_LocalAABBs; // 객체별 로컬 AABB 저장

std::vector<Light> g_Lights;

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowPosition(500, 50);
	glutInitWindowSize(Window_width, Window_height);
	glutCreateWindow("Example29, 30");

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

	// delta time
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
	// Draw Background
	if (BackgroundTextureID != 0) {
		glDisable(GL_DEPTH_TEST);

		glBindVertexArray(VAO_background);
		glUniform1i(FigureTypeID, Figure_Type::BACKGROUND);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, BackgroundTextureID);
		glUniform1i(TextureUniformID, 0);
		glUniform1i(UseTextureUniformID, 1);
		glUniform1f(ShininessID, 1.0f);

		glDrawElements(GL_TRIANGLES, Background_Index.size(), GL_UNSIGNED_INT, 0);

		glEnable(GL_DEPTH_TEST);
	}

	// Draw Axis
	glBindVertexArray(VAO_axis);
	glUniform1i(FigureTypeID, Figure_Type::AXIS);
	glLineWidth(2.0f);
	glDrawElements(GL_LINES, Axis_Index.size(), GL_UNSIGNED_INT, 0);

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

	// Draw OBJ Models
	for (const auto& file : g_OBJ_Files) {
		for (const auto& object : file.objects) {
			if (object.name.substr(0, 4) == "Snow") {
				continue;
			}
			glBindVertexArray(object.VAO);

			if (object.textureID != 0) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, object.textureID);
				glUniform1i(glGetUniformLocation(shaderProgramID, "texture1"), 0);
				glUniform1i(glGetUniformLocation(shaderProgramID, "useTexture"), 1);
			}
			else {
				glUniform1i(glGetUniformLocation(shaderProgramID, "useTexture"), 0);
			}

			GLuint currentFigureType;
			Type_distinction(object.name, currentFigureType);
			glUniform1i(FigureTypeID, currentFigureType);

			glUniform1f(ShininessID, object.shininess);

			// 텍스처 바인딩 디버그
			if (object.textureID != 0) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, object.textureID);
				glUniform1i(glGetUniformLocation(shaderProgramID, "texture1"), 0);
				glUniform1i(glGetUniformLocation(shaderProgramID, "useTexture"), 1);

				static bool debugPrinted = false;
				if (!debugPrinted) {
					std::cout << "Drawing " << object.name << " with texture ID: " << object.textureID << std::endl;
					GLint boundTex;
					glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex);
					std::cout << "  Bound texture: " << boundTex << std::endl;

					GLint useTexLoc = glGetUniformLocation(shaderProgramID, "useTexture");
					GLint tex1Loc = glGetUniformLocation(shaderProgramID, "texture1");
					std::cout << "  useTexture location: " << useTexLoc << std::endl;
					std::cout << "  texture1 location: " << tex1Loc << std::endl;

					if (object.name == "Cone") debugPrinted = true; // Cone까지 확인 후 멈춤
				}
			}
			else {
				glUniform1i(glGetUniformLocation(shaderProgramID, "useTexture"), 0);
			}

			glDrawElements(GL_TRIANGLES, object.indices.size(), GL_UNSIGNED_INT, 0);
		}
	}
}

void KeyBoard(unsigned char key, int x, int y) {
	keyStates[key] = true;
	switch (key) {
	case 'x':
		Cube_Rotation_Factor.x += 10.0f;
		Cone_Rotation_Factor.x += 10.0f;

		break;
	case 'X':
		Cube_Rotation_Factor.x -= 10.0f;
		Cone_Rotation_Factor.x -= 10.0f;

		break;
	case 'y':
		Cube_Rotation_Factor.y += 10.0f;
		Cone_Rotation_Factor.y += 10.0f;

		break;
	case 'Y':
		Cube_Rotation_Factor.y -= 10.0f;
		Cone_Rotation_Factor.y -= 10.0f;

		break;

	case 's':
		Cube_Rotation_Factor = glm::vec3(0.0f);
		Cone_Rotation_Factor = glm::vec3(0.0f);
		Cube_Rotation_by_Axis = glm::vec3(0.0f);
		Cone_Rotation_by_Axis = glm::vec3(0.0f);
		ShowCube = true;

		break;

	case 'c':
		ShowCube = true;
		
		std::cout << "Showing Cube\n";
		break;
	case 'p':
		ShowCube = false;

		std::cout << "Showing Pyramid\n";
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

void INIT_BUFFER() {
	std::vector<std::string> obj_filenames = {
		"Figures.obj",
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
			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex_glm), (GLvoid*)offsetof(Vertex_glm, texCoord));  // 텍스처 좌표
			glEnableVertexAttribArray(3);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.IBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, object.indices.size() * sizeof(unsigned int), object.indices.data(), GL_STATIC_DRAW);

			ComposeTexture(object);

			std::cout << "  L  Buffered Object: " << object.name << ", VAO: " << object.VAO << "\n";

			// 각 객체의 로컬 AABB 계산 및 저장
			g_LocalAABBs[object.name] = CalculateAABB(object.vertices);
			std::cout << "     Calculated AABB for " << object.name << "\n";
		}
	}
	glBindVertexArray(0);

	// Create Light Source Buffers
	Light light1(glm::vec3(10.0f, 20.0f, 10.0f));
	light1.intensity = 15.0f;
	g_Lights.push_back(light1);

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

	InitBackground();
	std::cout << "Background setup completed\n";
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

			auto pushVertex = [&](int vi, int ti, int ni) {
				Vertex_glm vertex;
				vertex.position = temp_vertices[vi - 1];
				vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);

				// UV 좌표 추가
				if (!temp_uvs.empty() && ti > 0) {
					vertex.texCoord = temp_uvs[ti - 1];
				}
				else {
					vertex.texCoord = glm::vec2(0.0f, 0.0f);
				}

				if (!temp_normals.empty() && ni > 0) {
					vertex.normal = temp_normals[ni - 1];
				}
				else {
					vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
				}
				currentObject->vertices.push_back(vertex);
				currentObject->indices.push_back(currentObject->vertices.size() - 1);
				};

			if (matches == 9) {
				pushVertex(vertexIndex[0], uvIndex[0], normalIndex[0]);
				pushVertex(vertexIndex[1], uvIndex[1], normalIndex[1]);
				pushVertex(vertexIndex[2], uvIndex[2], normalIndex[2]);
			}
			else if (matches == 12) {
				pushVertex(vertexIndex[0], uvIndex[0], normalIndex[0]);
				pushVertex(vertexIndex[1], uvIndex[1], normalIndex[1]);
				pushVertex(vertexIndex[2], uvIndex[2], normalIndex[2]);
				pushVertex(vertexIndex[0], uvIndex[0], normalIndex[0]);
				pushVertex(vertexIndex[2], uvIndex[2], normalIndex[2]);
				pushVertex(vertexIndex[3], uvIndex[3], normalIndex[3]);
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
GLuint LoadTexture(const char* filename) {
	stbi_set_flip_vertically_on_load(true);

	GLuint textureID;
	glGenTextures(1, &textureID);

	if (textureID == 0) {
		std::cerr << "Failed to generate texture ID for: " << filename << std::endl;
		return 0;
	}

	glBindTexture(GL_TEXTURE_2D, textureID);

	// 텍스처 파라미터 설정
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// 이미지 로드
	int width, height, nrChannels;
	unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);

	if (data) {
		GLenum format;
		if (nrChannels == 1)
			format = GL_RED;
		else if (nrChannels == 3)
			format = GL_RGB;
		else if (nrChannels == 4)
			format = GL_RGBA;
		else {
			std::cerr << "Unsupported channel count: " << nrChannels << " for " << filename << std::endl;
			stbi_image_free(data);
			return 0;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		std::cout << "v Loaded texture: " << filename << std::endl;
		std::cout << "  Size: " << width << "x" << height << ", Channels: " << nrChannels << ", TextureID: " << textureID << std::endl;

		// OpenGL 오류 확인
		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			std::cerr << "  OpenGL Error after loading texture: " << err << std::endl;
		}
	}
	else {
		std::cerr << "x Failed to load texture: " << filename << std::endl;
		std::cerr << "  stbi error: " << stbi_failure_reason() << std::endl;

		// 현재 작업 디렉토리 출력
		char cwd[1024];
		if (GetCurrentDirectoryA(sizeof(cwd), cwd)) {
			std::cerr << "  Current directory: " << cwd << std::endl;
		}

		glBindTexture(GL_TEXTURE_2D, 0);
		return 0;
	}

	stbi_image_free(data);
	glBindTexture(GL_TEXTURE_2D, 0);

	return textureID;
}
void ComposeTexture(Custom_OBJ& object) {
	if (object.name == "Cube") {
		object.textureID = LoadTexture("OBJ/CubeTexture.png");

		std::cout << "Cube Texture ID: " << object.textureID << std::endl;
	}
	else if (object.name == "Cone") {
		object.textureID = LoadTexture("OBJ/PyramidTexture.png");

		std::cout << "Cone Texture ID: " << object.textureID << std::endl;
	}
	else {
		object.textureID = 0;
	}
}
void InitBackground() {
	// 전체 화면을 덮는 사각형 (NDC 좌표계)
	Background_Vertex = {
		// Position (NDC)           Color                  Normal              TexCoord
		{ glm::vec3(-1.0f, -1.0f, 0.999f), glm::vec3(1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f) }, // 좌하
		{ glm::vec3(1.0f, -1.0f, 0.999f), glm::vec3(1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f) }, // 우하
		{ glm::vec3(1.0f,  1.0f, 0.999f), glm::vec3(1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f) }, // 우상
		{ glm::vec3(-1.0f,  1.0f, 0.999f), glm::vec3(1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f) }  // 좌상
	};

	Background_Index = {
		0, 1, 2,  // 첫 번째 삼각형
		0, 2, 3   // 두 번째 삼각형
	};

	glGenVertexArrays(1, &VAO_background);
	glGenBuffers(1, &VBO_background);
	glGenBuffers(1, &IBO_background);

	glBindVertexArray(VAO_background);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_background);
	glBufferData(GL_ARRAY_BUFFER, Background_Vertex.size() * sizeof(Vertex_glm), Background_Vertex.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_glm), (GLvoid*)offsetof(Vertex_glm, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_glm), (GLvoid*)offsetof(Vertex_glm, color));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_glm), (GLvoid*)offsetof(Vertex_glm, normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex_glm), (GLvoid*)offsetof(Vertex_glm, texCoord));
	glEnableVertexAttribArray(3);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO_background);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, Background_Index.size() * sizeof(unsigned int), Background_Index.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);

	// 배경 텍스처 로드
	BackgroundTextureID = LoadTexture("OBJ/background.png");

	std::cout << "Background initialized with texture ID: " << BackgroundTextureID << std::endl;
}

void MakeStaticMatrix() {
	Perspective_Matrix = glm::perspective(FOV, AspectRatio, NearClip, FarClip);
}
void MakeDynamicMatrix(float deltaTime) {
	View_Matrix = glm::lookAt(EYE + Camera_Transform, AT + Camera_Transform, UP);
	
	Model_Matrix = glm::mat4(1.0f);
	Model_Matrix = glm::scale(Model_Matrix, glm::vec3(5.0f, 5.0f, 5.0f));

	Cube_Rotation_by_Axis += Cube_Rotation_Factor * deltaTime;
	if (Cube_Rotation_by_Axis.x >= 360.0f) Cube_Rotation_by_Axis.x -= 360.0f;
	if (Cube_Rotation_by_Axis.y >= 360.0f) Cube_Rotation_by_Axis.y -= 360.0f;
	Cube_Matrix = glm::mat4(1.0f);
	Cube_Matrix = glm::rotate(Cube_Matrix, glm::radians(Cube_Rotation_by_Axis.x), glm::vec3(1.0f, 0.0f, 0.0f));
	Cube_Matrix = glm::rotate(Cube_Matrix, glm::radians(Cube_Rotation_by_Axis.y), glm::vec3(0.0f, 1.0f, 0.0f));

	Cone_Rotation_by_Axis += Cone_Rotation_Factor * deltaTime;
	if (Cone_Rotation_by_Axis.x >= 360.0f) Cone_Rotation_by_Axis.x -= 360.0f;
	if (Cone_Rotation_by_Axis.y >= 360.0f) Cone_Rotation_by_Axis.y -= 360.0f;
	Cone_Matrix = glm::mat4(1.0f);
	Cone_Matrix = glm::rotate(Cone_Matrix, glm::radians(Cone_Rotation_by_Axis.x), glm::vec3(1.0f, 0.0f, 0.0f));
	Cone_Matrix = glm::rotate(Cone_Matrix, glm::radians(Cone_Rotation_by_Axis.y), glm::vec3(0.0f, 1.0f, 0.0f));
}

void GetUniformLocations() {
	// static uniform variable
	PerspectiveMatrixID = glGetUniformLocation(shaderProgramID, "Perspective_Matrix");
	ViewMatrixID = glGetUniformLocation(shaderProgramID, "View_Matrix");
	ViewPosID = glGetUniformLocation(shaderProgramID, "viewPos");
	ModelMatrixID = glGetUniformLocation(shaderProgramID, "Model_Matrix");
	CubeMatrixID = glGetUniformLocation(shaderProgramID, "Cube_Matrix");
	ConeMatrixID = glGetUniformLocation(shaderProgramID, "Cone_Matrix");
	ShowCubeID = glGetUniformLocation(shaderProgramID, "Show_Cube");

	// Texture uniform
	TextureUniformID = glGetUniformLocation(shaderProgramID, "texture1");
	UseTextureUniformID = glGetUniformLocation(shaderProgramID, "useTexture");
	// 디버그 출력
	std::cout << "Texture uniform locations:" << std::endl;
	std::cout << "  texture1: " << TextureUniformID << std::endl;
	std::cout << "  useTexture: " << UseTextureUniformID << std::endl;

	// dynamic uniform variable
	FigureTypeID = glGetUniformLocation(shaderProgramID, "Figure_Type");
	ShininessID = glGetUniformLocation(shaderProgramID, "shininess");


}
void UpdateUniformMatrices() {
	glUniformMatrix4fv(PerspectiveMatrixID, 1, GL_FALSE, &Perspective_Matrix[0][0]);
	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &View_Matrix[0][0]);
	glUniform3fv(ViewPosID, 1, &EYE[0]);
	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Model_Matrix[0][0]);
	glUniformMatrix4fv(CubeMatrixID, 1, GL_FALSE, &Cube_Matrix[0][0]);
	glUniformMatrix4fv(ConeMatrixID, 1, GL_FALSE, &Cone_Matrix[0][0]);
	glUniform1i(ShowCubeID, ShowCube ? 1 : 0);

	if (PerspectiveMatrixID == -1) std::cerr << "Could not bind uniform Perspective_Matrix\n";
	if (ViewMatrixID == -1) std::cerr << "Could not bind uniform View_Matrix\n";
	if (ModelMatrixID == -1) std::cerr << "Could not bind uniform Model_Matrix\n";
	if (CubeMatrixID == -1) std::cerr << "Could not bind uniform Cube_Matrix\n";
	if (ConeMatrixID == -1) std::cerr << "Could not bind uniform Cone_Matrix\n";
	if (ShowCubeID == -1) std::cerr << "Could not bind uniform Show_Cube\n";

}
void ComposeOBJColor() {
	for (auto& file : g_OBJ_Files) {
		for (auto& object : file.objects) {
			if (object.name == "Temp") {
				glm::vec3 bodyColor(0.8f, 0.5f, 0.5f);
				for (auto& vertex : object.vertices) {
					vertex.color = bodyColor;
				}
			}
			
		}
	}

}

void Type_distinction(const std::string& object_name, GLuint& outTypeID) {
	if (object_name == "Cube") {
		outTypeID = Figure_Type::CUBE;
	}
	else if (object_name == "Cone") {
		outTypeID = Figure_Type::CONE;
	}
	else {
		outTypeID = Figure_Type::ETC;
	}
	
}
