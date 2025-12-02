#version 330 core
layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_color;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec2 in_texCoord;

out vec3 out_fragPos;
out vec3 out_normal;
out vec3 out_color;
out vec2 out_texCoord;

uniform mat4 Perspective_Matrix;
uniform mat4 View_Matrix;
uniform mat4 Model_Matrix;
uniform mat4 Cube_Matrix;
uniform mat4 Cone_Matrix;

uniform bool Show_Cube;

uniform int Figure_Type;

const int ETC = 99;

const int BACKGROUND = -3;
const int CUBE = 0;
const int CONE = 1;

void main()
{
    mat4 final_transform = mat4(1.0);

    if (Figure_Type == BACKGROUND) {
        gl_Position = vec4(in_position, 1.0);
        out_fragPos = in_position;
        out_normal = in_normal;
        out_color = in_color;
        out_texCoord = in_texCoord;
        return;
    }

    if (Figure_Type >= 0) {
        final_transform = final_transform * Model_Matrix;

        if (Figure_Type == CUBE) {
            final_transform *= Cube_Matrix;
        } else if (Figure_Type == CONE) {
            final_transform *= Cone_Matrix;
        }

        if ((Figure_Type == CUBE && !Show_Cube) || (Figure_Type == CONE && Show_Cube)) {
        gl_Position = vec4(0.0, 0.0, -1000.0, 1.0);
        return;
    }
        
        out_normal = mat3(transpose(inverse(final_transform))) * in_normal;
    } else {
        out_normal = in_normal;
    }



    out_fragPos = vec3(final_transform * vec4(in_position, 1.0));
    
    out_color = in_color;
    out_texCoord = in_texCoord;

    gl_Position = Perspective_Matrix * View_Matrix * vec4(out_fragPos, 1.0);
}