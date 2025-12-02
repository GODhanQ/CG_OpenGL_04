#version 330 core
out vec4 FragColor;

in vec3 out_fragPos;
in vec3 out_normal;
in vec3 out_color;
in vec2 out_texCoord;

struct Light {
    vec3 position;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
    float intensity;
};

uniform sampler2D texture1;
uniform bool useTexture;

#define MAX_LIGHTS 100
uniform Light lights[MAX_LIGHTS];
uniform int numLights;

uniform int Figure_Type;
uniform float shininess;
uniform vec3 viewPos;

const int BACKGROUND = -3;

void main()
{
    // 배경은 조명 계산 없이 텍스처만 출력
    if (Figure_Type == BACKGROUND) {
        if (useTexture) {
            FragColor = texture(texture1, out_texCoord);
        } else {
            FragColor = vec4(out_color, 1.0);
        }
        return;
    }

    // 일반 객체들 (Axis, Light 등)
    if (Figure_Type < 0) {
        FragColor = vec4(out_color, 1.0);
        return;
    }

    // 입력된 법선 벡터가 0일 경우를 대비한 방어 코드
    if (length(out_normal) < 0.001) {
        FragColor = vec4(out_color, 1.0);
        return;
    }
    
    vec3 N = normalize(out_normal);
    vec3 V = normalize(viewPos - out_fragPos);
    
    const float ambientf = 0.15;
    vec3 ambient = vec3(ambientf, ambientf, ambientf);
    vec3 lighting = vec3(0.0, 0.0, 0.0);

    for(int i = 0; i < numLights; i++)
    {
        vec3 L = lights[i].position - out_fragPos;
        float distance = length(L);
        L = normalize(L);

        // 감쇠 계산
        float attenuation = 1.0 / (lights[i].constant + lights[i].linear * distance + lights[i].quadratic * (distance * distance));
        
        // Diffuse
        float diff = max(dot(N, L), 0.0);
        vec3 diffuse = diff * lights[i].color;

        // Specular
        vec3 R = reflect(-L, N);
        float spec = 0.0;
        if (diff > 0.0) {
            spec = pow(max(dot(V, R), 0.0), shininess);
        }
        vec3 specular = spec * lights[i].color;

        lighting += attenuation * lights[i].intensity * (diffuse + specular);
    }

    // 텍스처 또는 버텍스 색상 선택
    vec3 baseColor = out_color;
    if (useTexture) {
        baseColor = texture(texture1, out_texCoord).rgb;
    }

    // 조명 적용
    vec3 final_color = (ambient + lighting) * baseColor;
    FragColor = vec4(final_color, 1.0);
}
