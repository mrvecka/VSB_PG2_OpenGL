#include "pch.h"
#include "tutorials.h"
#include "rasterizer.h"

float deg2rad(const float x)
{
	return x * float(M_PI) / 180.0f;
}

/* create a window and initialize OpenGL context */
int tutorial_1( const int width, const int height )
{
	Rasterizer rasterizer(width, height, deg2rad(45.0), Vector3(175, -140, 130), Vector3(0, 0, 35));
	rasterizer.InitDeviceAndScene("../../data/6887_allied_avenger_gi.obj");
	rasterizer.MainLoop();
	rasterizer.ReleaseDeviceAndScene();

	return S_OK;

}




