#pragma once
#include "surface.h"
#include "camera.h"
#include "structs.h"
#include "pch.h"

/*! \class Raytracer
\brief General ray tracer class.

\author Tomáš Fabián
\version 0.1
\date 2018
*/
class Raytracer
{
public:
	Raytracer( Camera cam, float fov_y,float near_plane,float far_plane);
	~Raytracer();

	int InitDeviceAndScene();
	int initGraph();
	int ReleaseDeviceAndScene();

	void LoadScene(int no_surfaces, std::vector<Surface *> & surfaces, std::vector<Material *> & materials);
	int Ui();

private:	
	std::vector<Surface *> surfaces_;
	std::vector<Material *> materials_;			
	
	RTcontext context = {0};
	RTbuffer outputBuffer = { 0 };
	RTvariable focal_length;
	RTvariable view_from;
	RTvariable M_c_w;

	Camera camera;
	float fov;

	bool unify_normals_{ true };
	void error_handler(RTresult code);

};
