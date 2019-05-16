#version 460 core


uniform sampler2D color_map;
uniform sampler2D shadow_map;
uniform vec2 screen_size = vec2( 1.0f / 640.0f,1.0f / 480.0f ); 

layout ( location = 0 ) out vec4 FragColor;

void main( void ) {
	vec2 tc = gl_FragCoord.xy * screen_size;
	vec4 color = texture( color_map, tc ).xyzw;
	vec4 shadow = texture( shadow_map, tc ).xyzw;
	FragColor = color;
}

