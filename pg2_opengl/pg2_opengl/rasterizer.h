#pragma once
#include "surface.h"
#include "camera.h"
#include "pch.h"
#include "utils.h"
#include "objloader.h"
#include "linmath.h"
#include "matrix4x4.h"
#include "glutils.h"

/*! \class Raytracer
\brief General ray tracer class.

\author Tom� Fabi�n
\version 0.1
\date 2018
*/
class Rasterizer
{
public:
	Rasterizer(const int width, const int height, const float fov_y, const Vector3 view_from, const Vector3 view_at);
	~Rasterizer();

	int InitDeviceAndScene(const char* filename);
	int initGraph();
	int MainLoop();
	int ReleaseDeviceAndScene();


	bool check_gl(const GLenum error = glGetError());

	//void GLAPIENTRY gl_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar * message, const void * user_param);
	//void framebuffer_resize_callback(GLFWwindow * window, int width, int height);
	//char * LoadShader(const char * file_name);
	//GLint CheckShader(const GLenum shader);

	void LoadScene(const std::string file_name);
	int Ui();

	int width;
	int height;

	GLuint vao = 0;
	GLuint vbo = 0;
	int no_triangles = 0;

	GLuint vertex_shader;
	GLuint fragment_shader;
	GLuint shader_program;

	GLFWwindow * window;

private:
	std::vector<Surface *> surfaces_;
	std::vector<Material *> materials_;

	//RTcontext context = { 0 };
	//RTbuffer outputBuffer = { 0 };
	//RTvariable focal_length;
	//RTvariable view_from;
	//RTvariable M_c_w;

	Camera camera;
	float fov;

	bool unify_normals_{ true };

};