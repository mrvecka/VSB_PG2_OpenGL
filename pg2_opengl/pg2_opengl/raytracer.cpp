#include "pch.h"
#include "raytracer.h"
#include "objloader.h"
#include "tutorials.h"
#include "mymath.h"
#include "omp.h"
#include "utils.h"

void Raytracer::error_handler(RTresult code)
{
	if (code != RT_SUCCESS)
	{
		const char* error_string;
		rtContextGetErrorString(context, code, &error_string);
		printf(error_string);
		throw std::runtime_error("RT_ERROR_UNKNOWN");
	}
}


Raytracer::Raytracer( Camera cam,float fov_y,float near_plane, float far_plane)
{
	InitDeviceAndScene();
	camera = cam;
	fov = fov_y;
}

Raytracer::~Raytracer()
{
	ReleaseDeviceAndScene();
}

int Raytracer::InitDeviceAndScene()
{	
	error_handler(rtContextCreate(&context));
	error_handler(rtContextSetRayTypeCount(context, 2));
	error_handler(rtContextSetEntryPointCount(context, 1));
	error_handler(rtContextSetMaxTraceDepth(context, 10));

	RTvariable output;
	error_handler(rtContextDeclareVariable(context, "output_buffer", &output));
	error_handler(rtBufferCreate(context, RT_BUFFER_OUTPUT, &outputBuffer));
	error_handler(rtBufferSetFormat(outputBuffer, RT_FORMAT_UNSIGNED_BYTE4));
	error_handler(rtBufferSetSize2D(outputBuffer, camera.width_, camera.height_));
	error_handler(rtVariableSetObject(output, outputBuffer));

	RTprogram primary_ray;
	error_handler(rtProgramCreateFromPTXFile(context, "optixtutorial.ptx", "primary_ray", &primary_ray));
	error_handler(rtContextSetRayGenerationProgram(context, 0, primary_ray));
	error_handler(rtProgramValidate(primary_ray));

	rtProgramDeclareVariable(primary_ray, "focal_length",
		&focal_length);
	rtProgramDeclareVariable(primary_ray, "view_from",
		&view_from);

	rtVariableSet3f(view_from, camera.view_from().x, camera.view_from().y, camera.view_from().z);
	rtVariableSet1f(focal_length, camera.focal_length());
	rtVariableSetMatrix3x3fv(M_c_w, 0, camera.M_c_w().data());

	RTprogram exception;
	error_handler(rtProgramCreateFromPTXFile(context, "optixtutorial.ptx", "exception", &exception));
	error_handler(rtContextSetExceptionProgram(context, 0, exception));
	error_handler(rtProgramValidate(exception));
	error_handler(rtContextSetExceptionEnabled(context, RT_EXCEPTION_ALL, 1));

	error_handler(rtContextSetPrintEnabled(context, 1));
	error_handler(rtContextSetPrintBufferSize(context, 4096));

	RTprogram miss_program;
	error_handler(rtProgramCreateFromPTXFile(context, "optixtutorial.ptx", "miss_program", &miss_program));
	error_handler(rtContextSetMissProgram(context, 0, miss_program));
	error_handler(rtProgramValidate(miss_program));

	return S_OK;
}

int Raytracer::ReleaseDeviceAndScene()
{
	error_handler(rtContextDestroy(context));
	return S_OK;
}

int Raytracer::initGraph() {
	error_handler(rtContextValidate(context));

	return S_OK;
}

void Raytracer::LoadScene(int no_surfaces,std::vector<Surface *> & surfaces, std::vector<Material *> & materials)
{

	int no_triangles = 0;

	for (auto surface : surfaces_)
	{
		no_triangles += surface->no_triangles();
	}

	RTgeometrytriangles geometry_triangles;
	error_handler(rtGeometryTrianglesCreate(context, &geometry_triangles));
	error_handler(rtGeometryTrianglesSetPrimitiveCount(geometry_triangles, no_triangles));

	RTbuffer vertex_buffer;
	error_handler(rtBufferCreate(context, RT_BUFFER_INPUT, &vertex_buffer));
	error_handler(rtBufferSetFormat(vertex_buffer, RT_FORMAT_FLOAT3));
	error_handler(rtBufferSetSize1D(vertex_buffer, no_triangles * 3));

	RTvariable normals;
	rtContextDeclareVariable(context, "normal_buffer", &normals);
	RTbuffer normal_buffer;
	error_handler(rtBufferCreate(context, RT_BUFFER_INPUT, &normal_buffer));
	error_handler(rtBufferSetFormat(normal_buffer, RT_FORMAT_FLOAT3));
	error_handler(rtBufferSetSize1D(normal_buffer, no_triangles * 3));

	RTvariable materialIndices;
	rtContextDeclareVariable(context, "material_buffer", &materialIndices);
	RTbuffer material_buffer;
	error_handler(rtBufferCreate(context, RT_BUFFER_INPUT, &material_buffer));
	error_handler(rtBufferSetFormat(material_buffer, RT_FORMAT_UNSIGNED_BYTE));
	error_handler(rtBufferSetSize1D(material_buffer, no_triangles));

	optix::float3* normalData = nullptr;
	optix::float3* vertexData = nullptr;
	optix::uchar1* materialData = nullptr;

	error_handler(rtBufferMap(normal_buffer, (void**)(&normalData)));
	error_handler(rtBufferMap(vertex_buffer, (void**)(&vertexData)));
	error_handler(rtBufferMap(material_buffer, (void**)(&materialData)));

	// surfaces loop
	int k = 0, l = 0;
	for (auto surface : surfaces_)
	{

		// triangles loop
		for (int i = 0; i < surface->no_triangles(); ++i, ++l)
		{
			Triangle & triangle = surface->get_triangle(i);

			materialData[l].x = (unsigned char)surface->get_material()->material_index;

			// vertices loop
			for (int j = 0; j < 3; ++j, ++k)
			{
				const Vertex & vertex = triangle.vertex(j);
				vertexData[k].x = vertex.position.x;
				vertexData[k].y = vertex.position.y;
				vertexData[k].z = vertex.position.z;

				normalData[k].x = vertex.normal.x;
				normalData[k].y = vertex.normal.y;
				normalData[k].z = vertex.normal.z;
			} // end of vertices loop

		} // end of triangles loop

	} // end of surfaces loop

	rtBufferUnmap(material_buffer);
	rtBufferUnmap(vertex_buffer);

	rtBufferValidate(material_buffer);
	rtVariableSetObject(materialIndices, material_buffer);
	rtBufferValidate(vertex_buffer);
	rtBufferValidate(normal_buffer);
	rtVariableSetObject(normals, normal_buffer);

	error_handler(rtGeometryTrianglesSetMaterialCount(geometry_triangles, materials_.size()));
	error_handler(rtGeometryTrianglesSetMaterialIndices(geometry_triangles, material_buffer, 0, sizeof(optix::uchar1), RT_FORMAT_UNSIGNED_BYTE));
	error_handler(rtGeometryTrianglesSetVertices(geometry_triangles, no_triangles * 3, vertex_buffer, 0, sizeof(optix::float3), RT_FORMAT_FLOAT3));

	RTprogram attribute_program;
	error_handler(rtProgramCreateFromPTXFile(context, "optixtutorial.ptx", "attribute_program", &attribute_program));
	error_handler(rtProgramValidate(attribute_program));
	error_handler(rtGeometryTrianglesSetAttributeProgram(geometry_triangles, attribute_program));

	error_handler(rtGeometryTrianglesValidate(geometry_triangles));

	// geometry instance
	RTgeometryinstance geometry_instance;
	error_handler(rtGeometryInstanceCreate(context, &geometry_instance));
	error_handler(rtGeometryInstanceSetGeometryTriangles(geometry_instance, geometry_triangles));
	error_handler(rtGeometryInstanceSetMaterialCount(geometry_instance, materials_.size()));

	for (Material* material : materials_) {
		RTmaterial rtMaterial;
		error_handler(rtMaterialCreate(context, &rtMaterial));

		RTprogram any_hit;
		error_handler(rtProgramCreateFromPTXFile(context, "optixtutorial.ptx", "any_hit", &any_hit));


		error_handler(rtProgramValidate(any_hit));
		error_handler(rtMaterialSetAnyHitProgram(rtMaterial, 0, any_hit));



		// acceleration structure
		RTacceleration sbvh;
		error_handler(rtAccelerationCreate(context, &sbvh));
		error_handler(rtAccelerationSetBuilder(sbvh, "Sbvh"));
		//error_handler( rtAccelerationSetProperty( sbvh, "vertex_buffer_name", "vertex_buffer" ) );
		error_handler(rtAccelerationValidate(sbvh));

		// geometry group
		RTgeometrygroup geometry_group;
		error_handler(rtGeometryGroupCreate(context, &geometry_group));
		error_handler(rtGeometryGroupSetAcceleration(geometry_group, sbvh));
		error_handler(rtGeometryGroupSetChildCount(geometry_group, 1));
		error_handler(rtGeometryGroupSetChild(geometry_group, 0, geometry_instance));
		error_handler(rtGeometryGroupValidate(geometry_group));

		RTvariable top_object;
		error_handler(rtContextDeclareVariable(context, "top_object", &top_object));
		error_handler(rtVariableSetObject(top_object, geometry_group));
	}

}
