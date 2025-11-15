#define _CRT_SECURE_NO_WARNINGS
#include "VarANDFunc_03_Shader26.h"

auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
std::default_random_engine dre(seed);
std::uniform_real_distribution<float> urd_0_1(0.0f, 1.0f);
std::uniform_int_distribution<int> uid_0_3(0, 3);

glm::mat4 Perspective_Matrix(1.0f), View_Matrix;
glm::mat4 Model_Matrix(1.0f);
glm::mat4 Cube_Matrix(1.0f), Pyramid_Matrix(1.0f);

std::vector<OBJ_File> g_OBJ_Files;
std::map<std::string, AABB> g_LocalAABBs; // 객체별 로컬 AABB 저장

GLuint VAO_orbit, VBO_orbit, IBO_orbit;;
std::vector<Light> g_Lights;
std::vector<Vertex_glm> orbit_vertices;
std::vector<unsigned int> orbit_indices;

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
	GLfloat rColor{ 0.2f }, gColor{ 0.2f }, bColor{ 0.4f };
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

	// Draw Models
	DrawModels();


	glBindVertexArray(0);
	glutSwapBuffers();
}
GLvoid Reshape(int w, int h) {
	MakeStaticMatrix();
	glUniformMatrix4fv(PerspectiveMatrixID, 1, GL_FALSE, &Perspective_Matrix[0][0]);
}
void DrawModels() {
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

	// Draw Light Source
	for (auto& light : g_Lights) {
		glBindVertexArray(light.VAO);

		glUniform1i(FigureTypeID, Figure_Type::LIGHT);
		glPointSize(5.0f);

		if (light.LightPosID != -1) {
			light.LightPosID = glGetUniformLocation(shaderProgramID, "lightPos");
			light.LightColorID = glGetUniformLocation(shaderProgramID, "lightColor");
		}
		
		if (Light_On) {
			glm::vec4 lightPos_world_vec4 = Model_Matrix * glm::vec4(light.light_vertex.position, 1.0f);
			glm::vec3 lightPos_world = glm::vec3(lightPos_world_vec4);

			glUniform3fv(light.LightPosID, 1, &lightPos_world[0]);
			glUniform3fv(light.LightColorID, 1, &light.light_color[0]);
		}
		else {
			glUniform3f(light.LightPosID, 0.0f, 0.0f, 0.0f);
			glUniform3f(light.LightColorID, 0.0f, 0.0f, 0.0f);
		}
		
		glDrawElements(GL_POINTS, 1, GL_UNSIGNED_INT, 0);

	}

	// Draw OBJ Models
	int box_face_count = 0;
	for (const auto& file : g_OBJ_Files) {
		for (const auto& object : file.objects) {
			/*if ((Draw_Cube && object.name.find("Cube") == std::string::npos) ||
				(!Draw_Cube && object.name.find("Pyramid") == std::string::npos)) {
				continue;
			}*/
			glBindVertexArray(object.VAO);

			GLuint Figure_Type;
			glm::mat4 Matrix(1.0f);
			Type_distinction(object.name, Figure_Type);
			glUniform1i(FigureTypeID, Figure_Type);

			glDrawElements(GL_TRIANGLES, object.indices.size(), GL_UNSIGNED_INT, 0);
		}
	}

}

void KeyBoard(unsigned char key, int x, int y) {
	keyStates[key] = true;
	switch (key) {
	case 'n':
		Draw_Cube = !Draw_Cube;

		std::cout << (Draw_Cube ? "Draw Cube\n" : "Draw Pyramid\n");
		break;
	case 'm':
		Light_On = !Light_On;

		std::cout << (Light_On ? "Light On\n" : "Light Off\n");
		break;
	case 'y':
		Rotation_Mode = (Rotation_Mode + 1) % 3;

		if (Rotation_Mode == 0)
			std::cout << "Rotation Stopped\n";
		else if (Rotation_Mode == 1)
			std::cout << "Rotation +Y Axis\n";
		else if (Rotation_Mode == 2)
			std::cout << "Rotation -Y Axis\n";
		break;
	case 'r':
		Revolution_Mode = (Revolution_Mode + 1) % 3;

		if (Revolution_Mode == 0)
			std::cout << "Revolution Stopped\n";
		else if (Revolution_Mode == 1)
			std::cout << "Revolution +Y Axis\n";
		else if (Revolution_Mode == 2)
			std::cout << "Revolution -Y Axis\n";
		break;
	case 'z':
		Light_Trasform.z += 0.5f;

		ComposeOribit();
		break;
	case 'Z':
		Light_Trasform.z -= 0.5f;

		ComposeOribit();
		break;
	case 'c':
		light_color_template_index = (light_color_template_index + 1) % light_color_template.size();
		
		for (auto& light : g_Lights) {
			light.light_color = light_color_template[light_color_template_index];
			light.light_vertex.color = light.light_color;
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
	Light light1;
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
				unsigned int v_indices[] = { 0, 1, 2, 0, 2, 3 };
				for (int i = 0; i < 6; ++i) {
					int idx = v_indices[i];
					pushVertex(vertexIndex[idx], normalIndex[idx]);
				}
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
void MakeDynamicMatrix() {
	// delta time
	static auto lastTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
	lastTime = currentTime;

	// Camera Translation
	// EYE += Camera_Transform_Factor * deltaTime * Camera_Movement_Factor_Scale;

	Model_Scale = glm::vec3(5.0f, 5.0f, 5.0f);

	Model_Matrix = glm::mat4(1.0f);
	Model_Matrix = glm::scale(Model_Matrix, Model_Scale);
	Model_Matrix = glm::translate(Model_Matrix, Model_Transform);

	if (Rotation_Mode == 1) {
		Cube_Rotation_Angle += Cube_Rotation_Factor * deltaTime;
		Pyramid_Rotation_Angle += Pyramid_Rotation_Factor * deltaTime;
	}
	else if (Rotation_Mode == 2) {
		Cube_Rotation_Angle -= Cube_Rotation_Factor * deltaTime;
		Pyramid_Rotation_Angle -= Pyramid_Rotation_Factor * deltaTime;
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

	Cube_Matrix = glm::mat4(1.0f);
	if (cubeObject) {
		glm::mat4 local = glm::mat4(1.0f);
		local = glm::translate(local, cubeObject->origin);
		local = glm::rotate(local, glm::radians(Cube_Rotation_Angle), glm::vec3(0.0f, 1.0f, 0.0f));
		local = glm::translate(local, -cubeObject->origin);
		Cube_Matrix = local;
	}

	Pyramid_Matrix = glm::mat4(1.0f);
	if (pyramidObject) {
		glm::mat4 local = glm::mat4(1.0f);
		local = glm::translate(local, pyramidObject->origin);
		local = glm::rotate(local, glm::radians(Pyramid_Rotation_Angle), glm::vec3(0.0f, 1.0f, 0.0f));
		local = glm::translate(local, -pyramidObject->origin);
		Pyramid_Matrix = local;
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
	CubeMatrixID = glGetUniformLocation(shaderProgramID, "Cube_Matrix");
	PyramidMatrixID = glGetUniformLocation(shaderProgramID, "Pyramid_Matrix");
	ViewPosID = glGetUniformLocation(shaderProgramID, "viewPos");


	// dynamic uniform variable
	FigureTypeID = glGetUniformLocation(shaderProgramID, "Figure_Type");
}
void UpdateUniformMatrices() {
	glUniformMatrix4fv(PerspectiveMatrixID, 1, GL_FALSE, &Perspective_Matrix[0][0]);
	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &View_Matrix[0][0]);
	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Model_Matrix[0][0]);
	glUniformMatrix4fv(CubeMatrixID, 1, GL_FALSE, &Cube_Matrix[0][0]);
	glUniformMatrix4fv(PyramidMatrixID, 1, GL_FALSE, &Pyramid_Matrix[0][0]);
	glUniform3fv(ViewPosID, 1, &EYE[0]);

	if (PerspectiveMatrixID == -1) std::cerr << "Could not bind uniform Perspective_Matrix\n";
	if (ViewMatrixID == -1) std::cerr << "Could not bind uniform View_Matrix\n";
	if (ModelMatrixID == -1) std::cerr << "Could not bind uniform Model_Matrix\n";
	if (CubeMatrixID == -1) std::cerr << "Could not bind uniform Cube_Matrix\n";
	if (PyramidMatrixID == -1) std::cerr << "Could not bind uniform Pyramid_Matrix\n";
	if (ViewPosID == -1) std::cerr << "Could not bind uniform viewPos\n";

}
void ComposeOBJColor() {
	for (auto& file : g_OBJ_Files) {
		for (auto& object : file.objects) {
			if (object.name == "Cube") {
				glm::vec3 bodyColor(0.8f, 0.5f, 0.5f);
				for (auto& vertex : object.vertices) {
					vertex.color = bodyColor;
				}
			}
			else if (object.name == "Pyramid") {
				glm::vec3 roofColor(0.5f, 0.8f, 0.5f);
				for (auto& vertex : object.vertices) {
					vertex.color = roofColor;
				}
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
	const float orbit_radius = glm::length(Light_Trasform);
	for (int i = 0; i <= orbit_segments; ++i) {
		float angle = 2.0f * glm::pi<float>() * static_cast<float>(i) / orbit_segments;
		Vertex_glm vertex;
		vertex.position.x = orbit_radius * cos(angle);
		vertex.position.y = 0.0f;
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
	if (object_name == "Cube") {
		outTypeID = Figure_Type::CUBE;
	}
	else if (object_name == "Pyramid") {
		outTypeID = Figure_Type::PYRAMID;
	}
	else if (object_name.substr(0, 6) == "Sphere") {
		outTypeID = Figure_Type::SPHERE;
	}
	else {
		outTypeID = Figure_Type::ETC;
	}
}
