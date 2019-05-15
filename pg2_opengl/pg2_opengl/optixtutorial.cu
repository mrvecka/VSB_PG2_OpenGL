#include "optixtutorial.h"

struct TriangleAttributes
{
	optix::float3 normal;
	optix::float2 texcoord;
	optix::float3 intersectionPoint;
	optix::float3 vectorToLight;
};

rtBuffer<optix::float3, 1> normal_buffer;
rtBuffer<optix::uchar4, 2> output_buffer;

rtDeclareVariable( rtObject, top_object, , );
rtDeclareVariable( uint2, launch_dim, rtLaunchDim, );
rtDeclareVariable( uint2, launch_index, rtLaunchIndex, );
rtDeclareVariable( PerRayData_radiance, shadow_ray_data, rtPayload, );
rtDeclareVariable( float2, barycentrics, attribute rtTriangleBarycentrics, );
rtDeclareVariable(TriangleAttributes, attribs, attribute attributes, "Triangle attributes");
rtDeclareVariable(optix::Ray, ray, rtCurrentRay, );
rtDeclareVariable(optix::float3, view_from, , );


RT_PROGRAM void attribute_program(void)
{
	const optix::float3 lightPossition = optix::make_float3(100, 100, 200);
	const optix::float2 barycentrics = rtGetTriangleBarycentrics();
	const unsigned int index = rtGetPrimitiveIndex();
	const optix::float3 n0 = normal_buffer[index * 3 + 0];
	const optix::float3 n1 = normal_buffer[index * 3 + 1];
	const optix::float3 n2 = normal_buffer[index * 3 + 2];

	attribs.normal = optix::normalize(n1 * barycentrics.x + n2 * barycentrics.y + n0 * (1.0f - barycentrics.x - barycentrics.y));

	if (optix::dot(ray.direction, attribs.normal) > 0) {
		attribs.normal *= -1;
	}

	attribs.intersectionPoint = optix::make_float3(ray.origin.x + ray.tmax * ray.direction.x,
		ray.origin.y + ray.tmax * ray.direction.y,
		ray.origin.z + ray.tmax * ray.direction.z);

	attribs.vectorToLight = lightPossition - attribs.intersectionPoint;
}

RT_PROGRAM void primary_ray( void )
{

	optix::Ray ray(view_from, attribs.vectorToLight, 0, 0.01f);

	PerRayData_radiance prd;
	rtTrace(top_object, ray, prd);

	//prd.visible should be set
	// access to buffers within OptiX programs uses a simple array syntax	
	output_buffer[launch_index] = optix::make_uchar4(prd.attenuation.x*255.0f, prd.attenuation.y*255.0f, prd.attenuation.z*255.0f, 255);
}



RT_PROGRAM void any_hit(void)
{
	shadow_ray_data.attenuation  = optix::make_float3(0.5f,0.5f,0.5f);
	rtTerminateRay();
}



/* may access variables declared with the rtPayload semantic in the same way as closest-hit and any-hit programs */
RT_PROGRAM void miss_program( void )
{
	shadow_ray_data.visible = 0.0f;
}

RT_PROGRAM void exception( void )
{
	const unsigned int code = rtGetExceptionCode();
	rtPrintf( "Exception 0x%X at (%d, %d)\n", code, launch_index.x, launch_index.y );
	rtPrintExceptionDetails();
	output_buffer[launch_index] = uchar4{ 255, 0, 255, 0 };
}


