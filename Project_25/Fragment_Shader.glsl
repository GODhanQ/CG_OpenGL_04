#version 330 core

in vec3 out_fragPos;
in vec3 out_normal;
in vec3 out_color;

out vec4 FragColor;

uniform int Figure_Type;
uniform vec3 lightPos, lightColor, viewPos;

void main()
{
    if (Figure_Type <= 0) { // LIGHT, AXIS, ORBIT
        FragColor = vec4(out_color, 1.0);
        return;
    }

    // ambient
    vec3 ambient = 0.3 * lightColor;

    // normal
    vec3 N = normalize(out_normal);
    if (length(N) < 1e-6) N = vec3(0.0, 1.0, 0.0);

    // light direction
    vec3 L = normalize(lightPos - out_fragPos);

    // diffuse
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * lightColor;

    // specular (Phong)
    float shininess = 128.0;
    vec3 V = normalize(viewPos - out_fragPos);
    vec3 R = reflect(-L, N);
    float spec = 0.0;
    if (diff > 0.0) {
        spec = pow(max(dot(V, R), 0.0), shininess);
    }
    vec3 specular = spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * out_color;
    FragColor = vec4(result, 1.0);
}
