#version 330
uniform vec4 surface_ambient_color;
uniform vec4 light_ambient_color;
uniform vec4 surface_diffuse_color;
uniform vec4 light_diffuse_color;
uniform vec4 surface_specular_color;
uniform vec4 light_specular_color;
uniform vec4 light_position;

uniform mat4 view;/* la matrice de "la caméra" */

/* unité de texture 2D référençant la texture que je souhaite
 * éventuellement utiliser. */
uniform sampler2D my_texture;

/* unité de texture 2D référençant la texture de normal map que je souhaite
 * éventuellement utiliser. */
uniform sampler2D my_nm_texture;

/* variable indiquant que je souhaite (ou non) utiliser une texture
 * dans mon rendu. */
uniform bool use_texture;

/* variable indiquant que je souhaite (ou non) utiliser une texture
 * pour perturber la map des normales. */
uniform bool use_nm_texture;


in  vec3 modnormal;
in  vec4 modpos;
/* récupérer la sortie du vertex shader transmettant la coordonnée de
 * texture (uv-map) depuis le vertex shader vers le fragment shader */
in  vec2 vsoTexCoord;

out vec4 fragColor;

void main() {
  vec4 lp = view * light_position;
  lp = lp / lp.w;
  vec3 light_direction = normalize((modpos - lp).xyz);
  float intensite_lumiere_diffuse = 1.0;

  /* gestion éventuelle d'une normal map, ici pour le plan horizontal */
  vec3 normal = modnormal;
  if(use_nm_texture) {
    normal = vec3(normal.x + texture(my_nm_texture, vsoTexCoord).r, normal.y, normal.z + texture(my_nm_texture, vsoTexCoord).g);
    normal.xyz += 0.02 * (2.0 * texture(my_nm_texture, vsoTexCoord).rbg - vec3(1.0));
    normal = normalize(normal);
  }
  intensite_lumiere_diffuse = clamp(dot(normal, -light_direction), 0.0, 1.0);
  vec4 ambient_color = light_ambient_color * surface_ambient_color;
  vec4 diffuse_color = intensite_lumiere_diffuse * light_diffuse_color * surface_diffuse_color;
  vec3 R = normalize(reflect(light_direction, normal)); 
  vec3 V = vec3(0.0, 0.0, -1.0);
  float intensite_lumiere_speculaire = pow(clamp(dot(R, -V), 0.0, 1.0), 10.0);
  vec4 specular_color = intensite_lumiere_speculaire * light_specular_color * surface_specular_color;
  fragColor = 0.2 * ambient_color + 0.8 * diffuse_color + specular_color;
  /* si je demande à utiliser une texture, multiplier la couleur
     calculée par cette couleur extraite de la texture en utilisant le
     sampler2D (unité de texture 2D) et en piochant à la coordonnée de
     texture récupérée depuis le vertex shader. */
  if(use_texture)
    fragColor *= texture(my_texture, vsoTexCoord);
}
