$input v_uv0, v_uv1, v_color, v_fogFactor

#include <bgfx_shader.sh>

SAMPLER2D(s_tex0, 0);
SAMPLER2D(s_tex1, 1);

uniform vec4 u_fragParams;   // x = useTexture, y = useLightmap, z = alphaRef, w = fogEnable
uniform vec4 u_fogColor;

void main()
{
    vec4 texColor = (u_fragParams.x > 0.5) ? texture2D(s_tex0, v_uv0) : vec4(1.0, 1.0, 1.0, 1.0);
    vec4 c = texColor * v_color;

    if (c.a < u_fragParams.z) {
        discard;
    }

    if (u_fragParams.y > 0.5) {
        c.rgb = c.rgb * texture2D(s_tex1, v_uv1).rgb;
    }

    if (u_fragParams.w > 0.5) {
        c.rgb = mix(u_fogColor.rgb, c.rgb, v_fogFactor);
    }

    gl_FragColor = c;
}
