#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;

uniform int Figure_Type;
uniform mat4 Perspective_Matrix, View_Matrix;
uniform mat4 Model_Matrix;
uniform mat4 Body_Matrix;
uniform mat4 LeftArm_Matrix, RightArm_Matrix;
uniform mat4 LeftLeg_Matrix, RightLeg_Matrix;
uniform mat4 Door_Matrix;

const int BOX_WALL = 5;
const int BOX_DOOR = 6;
const int ETC = 7;

out vec3 out_color;

void main()
{
    vec4 aPos = vec4(in_position, 1.0);
    
    if (Figure_Type >= 0 && Figure_Type < 50) {
        switch(Figure_Type) {
        case 0:             // Body
            aPos = Body_Matrix * aPos;
            break;
        case 1:             // Left Arm
            aPos = LeftArm_Matrix * aPos;
            break;
        case 2:             // Right Arm
            aPos = RightArm_Matrix * aPos;
            break;
        case 3:             // Left Leg
            aPos = LeftLeg_Matrix * aPos;
            break;
        case 4:             // Right Leg
            aPos = RightLeg_Matrix * aPos;
            break;
        }

        
        aPos = Model_Matrix * aPos;
    }

    if (Figure_Type == 51) {
        aPos = Door_Matrix * aPos;
    }

    gl_Position = Perspective_Matrix * View_Matrix * aPos;
    out_color = in_color;
}
