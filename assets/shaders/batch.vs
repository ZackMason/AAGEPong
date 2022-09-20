#version 460 core

#include <batch.glsl>

out vec2 voPos;
out vec2 voUV;
flat out int voTexture;

void main()
{
    voUV = aUV.xy;
    voTexture = int(aUV.z);
    voPos = aPos.xy * 2.0 - 1.0;
    //voPos.y = 1.0 - voPos.y;
        
    gl_Position = vec4(voPos, -0.001, 1);
}
