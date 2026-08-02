vec2 v_uv0       : TEXCOORD0 = vec2(0.0, 0.0);
vec2 v_uv1       : TEXCOORD1 = vec2(0.0, 0.0);
vec4 v_color     : COLOR0    = vec4(1.0, 1.0, 1.0, 1.0);
float v_fogFactor : TEXCOORD2 = 1.0;

vec3 a_position  : POSITION;
vec2 a_texcoord0 : TEXCOORD0;
vec4 a_color0    : COLOR0;
vec4 a_normal    : NORMAL;
ivec2 a_texcoord1 : TEXCOORD1;
