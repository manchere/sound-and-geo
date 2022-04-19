# Sound_and_Geometry
![Preview](preview/sound_geo.gif)

This is a code that use the Gl4Dummies graphics library to render 3D geometry and move with respect to the sound given.
The speed of the 3D objects is controlled by the sound

### Applying textures to objects

```c++
  /* activer la l'unité 1 pour y stocker une texture de normal map */
  glActiveTexture(GL_TEXTURE1);
  /* binder la texture _texId[2] (normal map de brick) pour l'utiliser sur l'unité 1 */
  glBindTexture(GL_TEXTURE_2D, _texId[2]);
  /* informer le programme GPU _pId que ma texture est celle posée à l'unité 1 */
  glUniform1i(glGetUniformLocation(_pId, "my_nm_texture"), 1 /* le 1 correspond à GL_TEXTURE1 */);
```


