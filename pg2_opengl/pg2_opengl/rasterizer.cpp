#include "pch.h"
#include "rasterizer.h"

void CreateBindlessTexture(GLuint & texture, GLuint64 & handle, const int width, const int height, unsigned char * data)
{
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture); // bind empty texture object to the target
	// set the texture wrapping/filtering options
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// copy data from the host buffer
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0); // unbind the newly created texture from the target
	handle = glGetTextureHandleARB(texture); // produces a handle representing the texture in a shader function
	glMakeTextureHandleResidentARB(handle);
}

Rasterizer::Rasterizer(const int width, const int height, const float fov_y, const Vector3 view_from, const Vector3 view_at,float near_plane,float far_plane)
{
	camera = Camera(width, height, fov_y, view_from, view_at,near_plane,far_plane);
	fov = fov_y;
	this->width = width;
	this->height = height;

}

Rasterizer::~Rasterizer()
{
	ReleaseDeviceAndScene();
}

bool Rasterizer::check_gl(const GLenum error)
{
	if (error != GL_NO_ERROR)
	{
		//const GLubyte * error_str;
		//error_str = gluErrorString( error );
		//printf( "OpenGL error: %s\n", error_str );

		return false;
	}

	return true;
}

/* glfw callback */
void glfw_callback(const int error, const char * description)
{
	printf("GLFW Error (%d): %s\n", error, description);
}

/* OpenGL messaging callback */
void GLAPIENTRY gl_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar * message, const void * user_param)
{
	printf("GL %s type = 0x%x, severity = 0x%x, message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "Error" : "Message"),
		type, severity, message);
}

/* invoked when window is resized */
void framebuffer_resize_callback(GLFWwindow * window, int width, int height)
{
	glViewport(0, 0, width, height);
}

/* load shader code from text file */
char * LoadShader(const char * file_name)
{
	FILE * file = fopen(file_name, "rt");

	if (file == NULL)
	{
		printf("IO error: File '%s' not found.\n", file_name);

		return NULL;
	}

	size_t file_size = static_cast<size_t>(GetFileSize64(file_name));
	char * shader = NULL;

	if (file_size < 1)
	{
		printf("Shader error: File '%s' is empty.\n", file_name);
	}
	else
	{
		/* v glShaderSource nezadáváme v posledním parametru délku,
		takže øetìzec musí být null terminated, proto +1 a reset na 0*/
		shader = new char[file_size + 1];
		memset(shader, 0, sizeof(*shader) * (file_size + 1));

		size_t bytes = 0; // poèet již naètených bytù

		do
		{
			bytes += fread(shader, sizeof(char), file_size, file);
		} while (!feof(file) && (bytes < file_size));

		if (!feof(file) && (bytes != file_size))
		{
			printf("IO error: Unexpected end of file '%s' encountered.\n", file_name);
		}
	}

	fclose(file);
	file = NULL;

	return shader;
}

/* check shader for completeness */
GLint CheckShader(const GLenum shader)
{
	GLint status = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	printf("Shader compilation %s.\n", (status == GL_TRUE) ? "was successful" : "FAILED");

	if (status == GL_FALSE)
	{
		int info_length = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_length);
		char * info_log = new char[info_length];
		memset(info_log, 0, sizeof(*info_log) * info_length);
		glGetShaderInfoLog(shader, info_length, &info_length, info_log);

		printf("Error log: %s\n", info_log);

		SAFE_DELETE_ARRAY(info_log);
	}

	return status;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int Rasterizer::InitDeviceAndScene(const char* filename)
{
	glfwSetErrorCallback(glfw_callback);

	if (!glfwInit())
	{
		return(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 8);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);

	window = glfwCreateWindow(width, height, "PG2 OpenGL", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		return EXIT_FAILURE;
	}


	glfwSetKeyCallback(window, key_callback);

	glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		if (!gladLoadGL())
		{
			return EXIT_FAILURE;
		}
	}

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(gl_callback, nullptr);

	printf("OpenGL %s, ", glGetString(GL_VERSION));
	printf("%s", glGetString(GL_RENDERER));
	printf(" (%s)\n", glGetString(GL_VENDOR));
	printf("GLSL %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	check_gl();

	glEnable(GL_MULTISAMPLE);

	// map from the range of NDC coordinates <-1.0, 1.0>^2 to <0, width> x <0, height>
	glViewport(0, 0, width, height);
	// GL_LOWER_LEFT (OpenGL) or GL_UPPER_LEFT (DirectX, Windows) and GL_NEGATIVE_ONE_TO_ONE or GL_ZERO_TO_ONE
	//glClipControl( GL_UPPER_LEFT, GL_NEGATIVE_ONE_TO_ONE );

	const int no_surfaces = LoadOBJ(filename, surfaces_, materials_);
	this->no_triangles = 0;

	for (auto surface : surfaces_)
	{
		this->no_triangles += surface->no_triangles();
	}

	//GLfloat* vertices = new GLfloat[no_triangles *3*11];
	std::vector<MyVertex> vertices;
	std::vector<int> indices;
	// surfaces loop
	int k = 0, l = 0;
	for (auto surface : surfaces_)
	{

		// triangles loop
		for (int i = 0; i < surface->no_triangles(); ++i, ++l)
		{
			Triangle & triangle = surface->get_triangle(i);

			//// vertices loop
			for (int j = 0; j < 3; ++j, ++k)
			{
				const Vertex & vertex = triangle.vertex(j);
				int m_index = surface->get_material()->material_index;
				//printf("material index= %i\n", m_index);

				vertices.push_back(MyVertex(vertex, m_index));

			}

		} // end of triangles loop

	} // end of surfaces loop

	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	const char * vertex_shader_source = LoadShader("basic_shader.vert");
	glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
	glCompileShader(vertex_shader);
	SAFE_DELETE_ARRAY(vertex_shader_source);
	CheckShader(vertex_shader);

	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	const char * fragment_shader_source = LoadShader("basic_shader.frag");
	glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);
	glCompileShader(fragment_shader);
	SAFE_DELETE_ARRAY(fragment_shader_source);
	CheckShader(fragment_shader);

	shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);
	// TODO check linking

	const int no_vertices = no_triangles * 3; //count of points
	const int size = (vertices.size() * sizeof(MyVertex)); // count of elements in vector * size of one element = size of whole array
	const int vertex_stride = sizeof(MyVertex); // size of one MyVertex
	// optional index array

	vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	vbo = 0;
	glGenBuffers(1, &vbo); // generate vertex buffer object (one of OpenGL objects) and get the unique ID corresponding to that buffer
	glBindBuffer(GL_ARRAY_BUFFER, vbo); // bind the newly created buffer to the GL_ARRAY_BUFFER target
	glBufferData(GL_ARRAY_BUFFER, (vertices.size() * sizeof(MyVertex)), vertices.data(), GL_STATIC_DRAW); // copies the previously defined vertex data into the buffer's memory

	// vertex position
	//GLuint posAttrib = glGetAttribLocation(shader_program, "in_position");
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertex_stride, (void*)0);
	glEnableVertexAttribArray(0);

	// normal
	//GLuint normalAttrib = glGetAttribLocation(shader_program, "in_normal");
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertex_stride, (void*)(sizeof(float) * 3));
	glEnableVertexAttribArray(1);

	// color
	//GLuint colorAttrib = glGetAttribLocation(shader_program, "in_color");
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, vertex_stride, (void*)(sizeof(float) * 6));
	glEnableVertexAttribArray(2);

	// vertex texture coordinates
	//GLuint texCoordAttrib = glGetAttribLocation(shader_program, "in_texcoord");
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, vertex_stride, (void*)(sizeof(float) * 9));
	glEnableVertexAttribArray(3);

	//material index
	glVertexAttribIPointer(5, 1, GL_INT, vertex_stride, (void*)(sizeof(float) * 11));
	glEnableVertexAttribArray(5);


	GLMaterial * gl_materials = new GLMaterial[materials_.size()];
	int m = 0;
	for (const auto & material : materials_) {
		Texture * tex_diffuse = material->texture(Material::kDiffuseMapSlot);
		if (tex_diffuse) {
			GLuint id = 0;
			CreateBindlessTexture(id, gl_materials[m].tex_diffuse_handle, tex_diffuse->width(), tex_diffuse->height(), tex_diffuse->data());
			gl_materials[m].diffuse = Color3f{ 1.0f, 1.0f, 1.0f }; // white diffuse color
		}
		else {
			GLuint id = 0;
			GLubyte data[] = { 255, 255, 255, 255 }; // opaque white
			CreateBindlessTexture(id, gl_materials[m].tex_diffuse_handle, 1, 1, data); // white texture
			gl_materials[m].diffuse = material->diffuse();
		}
		m++;
	}
	GLuint ssbo_materials = 0;
	glGenBuffers(1, &ssbo_materials);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_materials);
	const GLsizeiptr gl_materials_size = sizeof(GLMaterial) * materials_.size();
	glBufferData(GL_SHADER_STORAGE_BUFFER, gl_materials_size, gl_materials, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_materials);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	//GLuint ebo = 0; // optional buffer of indices
	//glGenBuffers( 1, &ebo );
	//glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );
	//glBufferData( GL_ELEMENT_ARRAY_BUFFER, (indices.size() * sizeof(int)), indices.data(), GL_STATIC_DRAW );

	glPointSize(2.0f);
	glLineWidth(1.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
		
	//main loop
	//release device

	return EXIT_SUCCESS;
}

int Rasterizer::MainLoop()
{
	glUseProgram(shader_program);
	float a = deg2rad(45);
	while (!glfwWindowShouldClose(window))
	{
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // state setting function
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // state using function

		glBindVertexArray(vao);
		Matrix4x4 model;
		model.set(0, 0, cosf(a));
		model.set(0, 1, -sinf(a));
		model.set(1, 0, sinf(a));
		model.set(1, 1, cosf(a));
		a += 1e-2f;
		Matrix4x4 mvp = camera.projection()*camera.view()*model;
		SetMatrix4x4(shader_program, mvp.data(), "MVP");
		Matrix4x4 mv = camera.view()*model;
		SetMatrix4x4(shader_program, mvp.data(), "MV");

		glDrawArrays(GL_TRIANGLES, 0, no_triangles*3);
		//glDrawArrays( GL_POINTS, 0, 3 );
		//glDrawArrays( GL_LINE_LOOP, 0, 3 );
		//glDrawElements( GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0 ); // optional - render from an index buffer

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	return S_OK;
}

int Rasterizer::ReleaseDeviceAndScene()
{
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	glDeleteProgram(shader_program);

	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);

	glfwTerminate();
	return S_OK;
}



