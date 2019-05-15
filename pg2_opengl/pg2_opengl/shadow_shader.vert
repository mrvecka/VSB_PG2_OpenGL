#version 460 core
layout ( location = 0 ) in vec4 in_position_cs;

void main( void ) {

	gl_Position = in_position_cs;
}
