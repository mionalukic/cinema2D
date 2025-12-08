#version 330 core

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec4 inCol;
layout(location = 2) in vec2 inUV; 


out vec4 chCol;
out vec2 TexCoords;

uniform float offsetY;     
uniform float useOffset;  
uniform vec2 offsetSeat;   

uniform float doorAngle;  
uniform vec2 doorPivot; 
uniform int isDoor;



void main()
{
    vec2 pos = inPos;

    if (isDoor == 1)
    {
        // translate to pivot
        pos -= doorPivot;

        // rotate
        float cs = cos(doorAngle);
        float sn = sin(doorAngle);
        pos = vec2(
            pos.x * cs - pos.y * sn,
            pos.x * sn + pos.y * cs
        );

        // translate back
        pos += doorPivot;
    }

    // apply vertical slide only for overlay
    pos.y += offsetY * useOffset;

    // apply seat offset only for seats (offsetSeat = 0 for all other objects)
    pos += offsetSeat;

    gl_Position = vec4(pos, 0.0, 1.0);
    chCol = inCol;
    TexCoords = inUV;

 
}

