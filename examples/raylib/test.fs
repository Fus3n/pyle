#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform float u_time;
uniform vec2 u_resolution;

out vec4 finalColor;

void main() {
    vec4 texel = texture(texture0, fragTexCoord);

    float gray = dot(texel.rgb, vec3(0.299, 0.587, 0.114));
    float pulse = 0.5 + 0.5 * sin(u_time * 2.0);

    vec3 color = mix(texel.rgb, vec3(gray), pulse);
    finalColor = vec4(color, texel.a) * colDiffuse;
}
