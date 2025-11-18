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
uniform mat4 Tank_Matrix;
uniform mat4 Wall_Matrix;
uniform mat4 Robot_Matrix;

uniform int Figure_Type;

const int FLOOR = 0;
const int TANK = 1;
const int WALL = 2;
const int ROBOT = 3;

void main()
{
    mat4 final_transform = mat4(1.0);

    if (Figure_Type >= 0) {
        if (Figure_Type == FLOOR) {
            final_transform = Model_Matrix * Floor_Matrix;
        } else if (Figure_Type == TANK) {
            final_transform = Model_Matrix * Tank_Matrix;
        } else if (Figure_Type == WALL) {
            final_transform = Model_Matrix * Wall_Matrix;
        } else if (Figure_Type == ROBOT) {
            final_transform = Model_Matrix * Robot_Matrix;
        }
        
        else {
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