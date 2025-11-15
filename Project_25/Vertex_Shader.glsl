#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec3 in_normal;

uniform int Figure_Type;
uniform mat4 Perspective_Matrix, View_Matrix;
uniform mat4 Model_Matrix;
uniform mat4 Cube_Matrix, Pyramid_Matrix;

out vec3 out_fragPos;
out vec3 out_color;
out vec3 out_normal;

void main()
{
    vec4 aPos = vec4(in_position, 1.0);
    mat4 objectMat = mat4(1.0);
    
    if (Figure_Type == 1) objectMat = Cube_Matrix;
    else if (Figure_Type == 2) objectMat = Pyramid_Matrix;
    
    if (Figure_Type >= 0 || Figure_Type == -3) {
        aPos = objectMat * aPos;
        aPos = Model_Matrix * aPos;
    }

    // Normal
    if (Figure_Type > 0) {
        mat3 normalMat = transpose(inverse(mat3(Model_Matrix * objectMat)));
        out_normal = normalize(normalMat * in_normal);
    } else {
        out_normal = in_normal;
    }

    out_fragPos = vec3(aPos);
    out_color = in_color;

    gl_Position = Perspective_Matrix * View_Matrix * aPos;
}
