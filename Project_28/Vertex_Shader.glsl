#version 330 core
layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_color;
layout (location = 2) in vec3 in_normal;

out vec3 out_fragPos;
out vec3 out_normal;
out vec3 out_color;

uniform mat4 Model_Matrix;
uniform mat4 View_Matrix;
uniform mat4 Perspective_Matrix;

uniform mat4 Floor_Matrix;
uniform mat4 Snow_Matrix;
uniform mat4 Mercury_Matrix;
uniform mat4 Venus_Matrix;
uniform mat4 Earth_Matrix;

uniform int Figure_Type;

const int FLOOR = 0;
const int PYRAMID = 1;
const int MERCURY = 2;
const int VENUS = 3;
const int EARTH = 4;

const int SNOW = 95;

void main()
{
    mat4 final_transform = mat4(1.0);

    if (Figure_Type >= 0) {
        if (Figure_Type == FLOOR) {
            final_transform = Model_Matrix * Floor_Matrix;
        } else if (Figure_Type == MERCURY) {
            final_transform = Model_Matrix * Mercury_Matrix;
        } else if (Figure_Type == VENUS) {
            final_transform = Model_Matrix * Venus_Matrix;
        } else if (Figure_Type == EARTH) {
            final_transform = Model_Matrix * Earth_Matrix;
        } else if (Figure_Type == SNOW) {
            final_transform = Model_Matrix * Snow_Matrix;
        } else { // ETC (99) 및 기타 객체
            final_transform = Model_Matrix;
        }
        out_normal = mat3(transpose(inverse(final_transform))) * in_normal;
    } else {
        out_normal = in_normal;
    }

    out_fragPos = vec3(final_transform * vec4(in_position, 1.0));
    
    out_color = in_color;

    gl_Position = Perspective_Matrix * View_Matrix * vec4(out_fragPos, 1.0);
}