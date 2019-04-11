#version 460 core
layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;
layout (location = 3) in vec2 in_texcoord;

layout (location = 4) in mat4 MVP;
uniform mat4 MVP;

out vec3 ex_color;
out vec3 ex_normal;
out mat4 out_mvp;

void main( void )
{
	//gl_Position = vec4( in_position.x, -in_position.y, in_position.z, 1.0f );
	gl_Position = MVP * vec4(position.x, position.y, position.z, 1.0f);
	if(in_texcoord.x <0)
	{
		ex_color = (in_normal+vec3(1,1,1))*0.5f;
	}
	else
	{
		ex_color = (in_normal+vec3(1,1,1))*0.5f;;
	}
	ex_normal = in_normal;
	out_mvp = MVP;
}
