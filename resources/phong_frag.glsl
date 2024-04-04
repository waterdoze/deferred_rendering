#version 120

struct Light {
    vec3 position;
    vec3 color;
};

const int MAX_LIGHTS = 10;

uniform vec3 lightPositions[10];
uniform vec3 lightColors[10];
uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float s;

varying vec3 normal;
varying vec4 position;


void main()
{
    gl_FragColor = vec4(ka,1.0);
    vec3 n = normalize(normal);
    vec3 eVec = -1 * position.xyz;

    for (int i = 0; i < MAX_LIGHTS; i++) {

        vec3 lightVecNormalized = normalize(lightPositions[i] - position.xyz);
        vec3 hVec = normalize(normalize(eVec) + lightVecNormalized);
        vec3 color = lightColors[i] * (kd * max(0.0, dot(lightVecNormalized, n)) + ks * pow(max(0.0, dot(hVec, n)), s));

        float d = distance(normalize(lightPositions[i]), normalize(position.xyz));
        float a1r = 0.0429 * d;
        float a2r2 = 0.9857 * pow(d, 2);
        float attenuation = 1.0 / (1.0 + a1r + a2r2);

        gl_FragColor += vec4(color * attenuation, 1.0);
    }


}
