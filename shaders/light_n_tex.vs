#version 330

/* Ces entrées proviennent du CPU et sont attendues à l'emplacement 0, 1 et 2 */
layout(location = 0) in vec3 pos; /* position du sommet dans l'espace objet */
layout(location = 1) in vec3 normal; /* normale au sommet dans l'espace objet */
layout(location = 2) in vec2 texCoord; /* coordonnée de texture 2D du sommet */

uniform mat4 proj; /* la matrice de projection */
uniform mat4 model; /* la matrice modélisation-monde */
uniform mat4 view;/* la matrice de "la caméra" */
/* facteur multiplicatif de texture */
uniform float mult_tex_coord;

out vec3 modnormal;
out vec4 modpos;
/* nouvelle sortie, je transmets la coordonnée de texture (uv-map)
 * depuis le vertex shader vers le fragment shader */
out vec2 vsoTexCoord;

void main() {
  modnormal = normalize((transpose(inverse(view * model)) * vec4(normal, 0.0)).xyz);
  modpos = view * model * vec4(pos, 1.0);
  gl_Position = proj * modpos;
  vsoTexCoord = mult_tex_coord * texCoord;
}
