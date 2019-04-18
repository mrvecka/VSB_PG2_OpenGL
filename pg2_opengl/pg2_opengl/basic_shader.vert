#version 460 core
layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;
layout (location = 3) in vec2 in_texcoord;

uniform mat4 MVP;
uniform mat4 MV; 

out vec3 ex_color;
out vec3 ex_normal;

void main( void )
{
	//gl_Position = vec4( in_position.x, -in_position.y, in_position.z, 1.0f );
	gl_Position = MVP * vec4(in_position.x, in_position.y, in_position.z, 1.0f);
	if(in_texcoord.x <0)
	{
		ex_color = in_color;
	}
	else
	{
		ex_color = (in_normal+vec3(1,1,1))*0.5f;;
	}
	ex_color = in_color;
	vec3 unified_normal_es = normalize( ( MV * vec4( in_normal.xyz, 0.0f ) ).xyz );
	vec4 hit_es = MV * vec4(in_position.xyz,1.0f); // in_position = (nx, ny, nz, 0)
	vec3 omega_i_es = normalize( hit_es.xyz / hit_es.w );
	if ( dot( unified_normal_es, omega_i_es ) > 0.0f )
	{
		unified_normal_es *= -1.0f;
	}	ex_normal = unified_normal_es;
}
