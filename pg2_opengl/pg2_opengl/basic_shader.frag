#version 460 core

#extension GL_ARB_bindless_texture : require

in vec3 ex_color;
in vec3 ex_normal;
in vec2 ex_tex_coord;
flat in int ex_material_index;
struct Material
{
	vec3 diffuse;
	sampler2D tex_diffuse_handle;
};

layout (std430, binding = 0) readonly buffer Materials
{
	Material materials[];
};


out vec4 FragColor;

void main( void )
{
	FragColor =  vec4( materials[ex_material_index].diffuse.rgb *
texture( materials[ex_material_index].tex_diffuse_handle, ex_tex_coord ).rgb, 1.0f );
}
