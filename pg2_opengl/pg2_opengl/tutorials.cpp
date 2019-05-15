#include "pch.h"
#include "tutorials.h"
#include "rasterizer.h"

/* create a window and initialize OpenGL context */
int tutorial_1( const int width, const int height )
{
	Rasterizer rasterizer(width, height, deg2rad(45.0), Vector3(150, -500, 200), Vector3(0, 0, 35),1.0f,1000.0f);
	rasterizer.InitDeviceAndScene("../../data/6887_allied_avenger_gi.obj");
	rasterizer.MainLoop();
	rasterizer.ReleaseDeviceAndScene();


	//----- run shadow program for check ----
	/*rasterizer.InitShaderProgram();
	rasterizer.MainLoopShadow();
	rasterizer.ReleaseDeviceAndSceneShadow();*/

	return S_OK;

}




