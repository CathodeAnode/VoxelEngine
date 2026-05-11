#version 450 core

layout(location = 0) in vec3 aPosition;

layout(location = 1) in vec4 iModelCol0;
layout(location = 2) in vec4 iModelCol1;
layout(location = 3) in vec4 iModelCol2;
layout(location = 4) in vec4 iModelCol3;

layout(location = 5) in vec4 iColor;

uniform mat4 uViewProj;
uniform bool uAlwaysOnTop;

out vec4 vColor;

void main()
{
    mat4 model = mat4(
        iModelCol0,
        iModelCol1,
        iModelCol2,
        iModelCol3
    );

    vec4 worldPos = model * vec4(aPosition, 1.0);
    vec4 clipPos  = uViewProj * worldPos;

    // Optional always-on-top mode
    if (uAlwaysOnTop)
    {
        clipPos.z = -clipPos.w * 0.0001;
    }

    gl_Position = clipPos;
    vColor = iColor;
}