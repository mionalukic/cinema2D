#version 330 core

in vec4 chCol;
in vec2 TexCoords;

out vec4 outCol;

uniform vec4 seatColor;
uniform int isSeat;

uniform sampler2D texture1;

void main()
{
    if (isSeat == 1)
        outCol = texture(texture1, TexCoords);
    else
        outCol = chCol;
}

