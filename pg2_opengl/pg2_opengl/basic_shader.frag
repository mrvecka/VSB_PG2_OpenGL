#version 460 core

in vec3 ex_color;
in vec3 ex_normal;

out vec4 FragColor;

void main( void )
{
	FragColor = vec4( ex_color, 1.0f );
}
