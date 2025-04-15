
/* inclusion des entêtes de fonctions de création et de gestion de
 * fenêtres système ouvrant un contexte favorable à GL4dummies. Cette
 * partie est dépendante de la bibliothèque SDL2 */
#include <GL4D/gl4duw_SDL2.h>
#include <GL4D/gl4dm.h>
#include <GL4D/gl4dg.h>
/* pour la macro RGB */
#include <GL4D/gl4dp.h>
/* pour IMG_Load */
#include <SDL_image.h>
/* pour l'ensemble des fonctions liées au son */
#include <SDL_mixer.h>

static void init(void);
static void initAudio(const char * filename);
static void mixCallback(void *udata, Uint8 *stream, int len);
static void draw(void);
static void quit(void);

/* on créé une variable pour stocker l'identifiant du programme GPU */
GLuint _pId = 0;
/* on créé une variable pour stocker l'identifiant de la géométrie : un plan, un cube et une sphère GL4D */
GLuint _plan = 0;
GLuint _sphere = 0;
GLuint _cone = 0;
/* entiers (positifs) pour y stocker les identifiants de textures
 * OpenGL générées. */
GLuint _texId[] = { 0, 0, 0 };
/* pointeur vers la musique chargée par SDL2_Mixer */
static Mix_Music * _mmusic = NULL;
/* pour stocker la moyenne du son sur 1/44 de seconde (cherchez pourquoi 1/44) */
static double _moyenne_son = 0.0;

/*!\brief créé la fenêtre, un screen 2D effacé en noir et lance une
 *  boucle infinie.*/
int main(int argc, char ** argv) {
  /* tentative de création d'une fenêtre pour GL4Dummies */
  if(!gl4duwCreateWindow(argc, argv, /* args du programme */
			 "GL4Dummies' Crabes Danse", /* titre */
			 10, 10, 800, 800, /* x,y, largeur, heuteur */
			 GL4DW_SHOWN) /* état visible */) {
    /* ici si échec de la création souvent lié à un problème d'absence
     * de contexte graphique ou d'impossibilité d'ouverture d'un
     * contexte OpenGL (au moins 3.2) */
    return 1;
  }
  /* appeler init pour initialiser des paramètres GL et GL4D */
  init();
  /* appeler initAudio pour ouvrir l'audio, le fichier son et commencer à le jouer */
  initAudio("audio/noisestorm_crab.mp3");
  /* placer quit comme fonction à appeler au moment du exit */
  atexit(quit);
  /* placer draw comme fonction à appeler pour dessiner chaque frame */
  gl4duwDisplayFunc(draw);
  /* boucle infinie pour éviter que le programme ne s'arrête et ferme
   * la fenêtre immédiatement */
  gl4duwMainLoop();
  return 0;
}

/* initialise des paramètres GL et GL4D */
void init(void) {
  /* image directement en RAM de 2x2 pixels (texels) avec un gris, un
   * blanc en première ligne puis un blanc un gris en deuxième
   * ligne */
  GLuint pixels[] = { (GLuint)RGB(128, 128, 128), (GLuint)-1, (GLuint)-1, (GLuint)RGB(128, 128, 128) };
  /* set la couleur d'effacement OpenGL */
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  /* activer le test de profondeur */
  glEnable(GL_DEPTH_TEST);
  /* générer un plan en GL4D */
  _plan = gl4dgGenQuadf();
  /* générer un cube en GL4D */
  _sphere = gl4dgGenSpheref(5, 9);
  /* générer une sphere en GL4D */
  _cone = gl4dgGenConef(4,9);
  /* créer un programme GPU pour OpenGL (en GL4D) */
  _pId = gl4duCreateProgram("<vs>shaders/light_n_tex.vs", "<fs>shaders/light_n_tex.fs", NULL);
  /* créer dans GL4D une matrice qui s'appelle model ; matrice de
     modélisation qu'on retrouvera dans le vertex shader */
  gl4duGenMatrix(GL_FLOAT, "model");
  /* créer dans GL4D une matrice qui s'appelle view ; matrice de
     vue (point de vue) qu'on retrouvera dans le vertex shader */
  gl4duGenMatrix(GL_FLOAT, "view");
  /* créer dans GL4D une matrice qui s'appelle proj ; matrice de
     projection qu'on retrouvera dans le vertex shader */
  gl4duGenMatrix(GL_FLOAT, "proj");
  /* binder (mettre au premier plan, "en courante" ou "en active") la matrice proj */
  gl4duBindMatrix("proj");
  /* mettre la matrice identité (celle qui ne change rien) dans la matrice courante */
  gl4duLoadIdentityf();
  /* combiner la matrice courante avec une matrice de projection en
     perspective. Voir le support de cours pour les six paramètres :
     left, right, bottom, top, near, far */
  gl4duFrustumf(-1, 1, -1, 1, 1, 1000);

  /* Générer 3 identifiants de texture côté OpenGL (GPU) pour y
   * transférer des textures. */
  glGenTextures(3, _texId);
  /* binder la texture générée comme texture 2D côté GPU */
  glBindTexture(GL_TEXTURE_2D, _texId[0]);
  /* paramétrer quelques propriétés de texture : voir la doc OpenGL de glTexParameteri */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST /* mettre GL_LINEAR si interpolation linéaire */);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST /* mettre GL_LINEAR si interpolation linéaire */);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT /* REPEAT est le par défaut, mettre par exemple GL_MIRRORED_REPEAT pour une répitions en mirroir */);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT /* REPEAT est le par défaut, mettre par exemple GL_MIRRORED_REPEAT pour une répitions en mirroir */);
  /* fonction de transfert de données (texels) de CPU vers GPU :
   * fonction assez riche, voir sa doc */
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  /* à ce niveau pixels en RAM-CPU n'est plus nécessaire */
  /* dé-binder la texture, par mesure de protection de ses paramètres
   * et ses données */
  glBindTexture(GL_TEXTURE_2D, 0);

  /* binder la texture générée comme texture 2D côté GPU */
  glBindTexture(GL_TEXTURE_2D, _texId[1]);
  /* paramétrer quelques propriétés de texture : voir la doc OpenGL de glTexParameteri */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR /* on veut la lisser */);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR /* on veut la lisser */);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT /* on veut la rendre cyclique */);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT /* on veut la rendre cyclique */);
  /* bloc concernant le chargement de fichier image, sa conversion
   * dans notre format RGBA et son transfert vers le GPU */
  {
    /* utilisation de SDL2_image pour charger une image au format png */
    SDL_Surface * orig = IMG_Load("images/wood_maps/wood_color.png");
    /* si échec de chargement, mettre une texture avec un seul pixel blanc */
    if(orig == NULL) {
      GLuint blanc = -1; /* ou 0xFFFFFFFF */
      /* transfert du pixel blanc */
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &blanc);      
    } else {
      /* création d'une surface SDL (image) compatible avec mon format de transfert OpenGL */
      SDL_Surface * image_au_format_RGBA = SDL_CreateRGBSurface(0, orig->w, orig->h, 32, R_MASK, G_MASK, B_MASK, A_MASK);
      /* copie de orig vers image_au_format_RGBA en convertissant au format de image_au_format_RGBA */
      SDL_BlitSurface(orig, NULL, image_au_format_RGBA, NULL);
      /* plus besoin d'orig, libérer la mémoire */
      SDL_FreeSurface(orig);
      /* fonction de transfert de données (texels) RGBA de CPU vers GPU :
       * fonction assez riche, voir sa doc */
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_au_format_RGBA->w, image_au_format_RGBA->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_au_format_RGBA->pixels);
      /* plus besoin de image_au_format_RGBA, libérer la mémoire */
      SDL_FreeSurface(image_au_format_RGBA);
    }
  }
  /* à ce niveau pixels en RAM-CPU n'est plus nécessaire */
  /* dé-binder la texture, par mesure de protection de ses paramètres
   * et ses données */
  glBindTexture(GL_TEXTURE_2D, 0);

  /* pensez à faire une fonction car ici il y a beaucoup de répétitions */
  /* binder la texture générée comme texture 2D côté GPU */
  glBindTexture(GL_TEXTURE_2D, _texId[2]);
  /* paramétrer quelques propriétés de texture : voir la doc OpenGL de glTexParameteri */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR /* on veut la lisser */);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR /* on veut la lisser */);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT /* on veut la rendre cyclique */);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT /* on veut la rendre cyclique */);
  /* bloc concernant le chargement de fichier image, sa conversion
   * dans notre format RGBA et son transfert vers le GPU */
  {
    /* utilisation de SDL2_image pour charger une image au format png */
    SDL_Surface * orig = IMG_Load("images/wood_maps/wood_normal.png");
    /* si échec de chargement, mettre une texture avec un seul pixel noir */
    if(orig == NULL) {
      GLuint noir = 0;
      /* transfert du pixel noir */
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &noir);      
    } else {
      /* création d'une surface SDL (image) compatible avec mon format de transfert OpenGL */
      SDL_Surface * image_au_format_RGBA = SDL_CreateRGBSurface(0, orig->w, orig->h, 32, R_MASK, G_MASK, B_MASK, A_MASK);
      /* copie de orig vers image_au_format_RGBA en convertissant au format de image_au_format_RGBA */
      SDL_BlitSurface(orig, NULL, image_au_format_RGBA, NULL);
      /* plus besoin d'orig, libérer la mémoire */
      SDL_FreeSurface(orig);
      /* fonction de transfert de données (texels) RGBA de CPU vers GPU :
       * fonction assez riche, voir sa doc */
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_au_format_RGBA->w, image_au_format_RGBA->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_au_format_RGBA->pixels);
      /* plus besoin de image_au_format_RGBA, libérer la mémoire */
      SDL_FreeSurface(image_au_format_RGBA);
    }
  }
  /* à ce niveau pixels en RAM-CPU n'est plus nécessaire */
  /* dé-binder la texture, par mesure de protection de ses paramètres
   * et ses données */
  glBindTexture(GL_TEXTURE_2D, 0);
}

/*!\brief Cette fonction initialise les paramètres SDL_Mixer et charge
 *  le fichier audio.*/
static void initAudio(const char * filename) {
  int mixFlags = MIX_INIT_MP3 /* on veut une gestion du MP3 */, res;
  res = Mix_Init(mixFlags);
  if( (res & mixFlags) != mixFlags ) {
    fprintf(stderr, "Mix_Init: Erreur lors de l'initialisation de la bibliotheque SDL_Mixer\n");
    fprintf(stderr, "Mix_Init: %s\n", Mix_GetError());
    exit(3); /* commenter car il arrive qu'il gère mais qu'il dit que non ????? */
  }
  /* ouvrir l'audio en 44Khz, 16bits/échantillon, stéréo et 1024
   * packets soumis à la fois à la callBack */
  if(Mix_OpenAudio(44100, AUDIO_S16LSB, 2, 1024) < 0)
    exit(4);
  /* ouvrir le fichier audio passé en paramètre */
  if(!(_mmusic = Mix_LoadMUS(filename))) {
    fprintf(stderr, "Erreur lors du Mix_LoadMUS: %s\n", Mix_GetError());
    exit(5);
  }
  /* mise en place de la fonction callBack pendant le play */
  Mix_SetPostMix(mixCallback, NULL);
  /* si tu ne joues pas, joue une fois ! */
  if(!Mix_PlayingMusic())
    Mix_PlayMusic(_mmusic, 1);
}

/*!\brief Cette fonction est appelée quand l'audio est joué et met
 * dans \a stream les données audio de longueur \a len.
 * \param udata pour user data, données passées par l'utilisateur, ici NULL. 
 * \param stream flux de données audio. 
 * \param len longueur de \a stream. Elle doit correspondre au nombre
 * d'échantillons demandés x (fois) le nombre d'octects par
 * échantillon x (fois) le nombre de canaux (2 si stéréo). Dans
 * l'exemple ici : 1024x2x2 = 4096. Attention si stéréo, un
 * échantillon sur deux est pour chaque sortie (gauche/droite). */
static void mixCallback(void *udata, Uint8 *stream, int len) {
  Sint16 *s16 = (Sint16 *)stream;
  /* je me contente de moyenner les intensités du son */
  int i;
  double moyenne = 0.0;
  for(i = 0; i < len / 2 /* car 2 octets par échantillon */; ++i) {
    moyenne += (s16[i] < 0.0 ? -s16[i] : s16[i]) / (double)(1<<13);
  }
  moyenne = moyenne / i;
  _moyenne_son = moyenne < 1.0 ? moyenne : 1.0;
}

static double inter_frames_dt(void) {
  static double t0 = -1.0;
  double t = gl4dGetElapsedTime(), dt;
  if(t0 < 0.0) /* ça ne devrait arriver qu'au premier appel */
    t0 = t;
  dt = (t - t0) / 1000.0;
  /* pour la prochaine frame */
  t0 = t;
  return dt;
}

void draw(void) {
  int i;
  /* une variable pour stocker un angle qui s'incrémente */
  static float a = 0;
  /* quelques couleurs qu'on transmettra au program shader */
  const GLfloat rouge[] = {1.0f, 0.0f, 0.0f, 1.0f};
  const GLfloat blanc[] = {1.0f, 1.0f, 1.0f, 1.0f};
  const GLfloat vert_tres_clair[] = {0.7f, 1.0f, 0.7f, 1.0f};
  const GLfloat jaune_clair[] = {0.9f, 0.9f, 0.5f, 1.0f};
  const GLfloat bleu[] = {0.2f, 0.2f, 0.9f, 1.0f};
  static GLfloat position_lumiere[] = {2.0f, 3.5f, -5.5f, 1.0f};
  GLfloat amplified_amb_light[4], amplified_diff_light[4], amplified_spec_light[4], cam_pos[3];
  /* on bouge un peu la lumière */
  position_lumiere[0] =  6.0f * sin(a / 200.0f);
  position_lumiere[2] = -6.0f * cos(a / 200.0f);
  /* effacer le buffer de couleur (image) et le buffer de profondeur d'OpenGL */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  /* utiliser le programme GPU "_pId" */
  glUseProgram(_pId);
  /* mettre le facteur de multiplication de texture (répétition, voir le vertex shader) à 1 */
  glUniform1f(glGetUniformLocation(_pId, "mult_tex_coord"), 1.0f);
  /* binder (mettre au premier plan, "en courante" ou "en active") la
     matrice view */
  gl4duBindMatrix("view");
  /* mettre la matrice identité (celle qui ne change rien) dans la matrice courante */
  gl4duLoadIdentityf();
  /* position de la caméra */
  cam_pos[0] = 6.0f * sin(-a / 1000.0);
  cam_pos[1] = 2.0f;
  cam_pos[2] = 6.0f * cos(-a / 1000.0);
  /* composer (multiplication à droite) avec une matrice fabriquée par LookAt */ 
  gl4duLookAtf(cam_pos[0], cam_pos[1], cam_pos[2], 0, 0, 0, 0, 1, 0);
  /* binder (mettre au premier plan, "en courante" ou "en active") la
     matrice model */
  gl4duBindMatrix("model");

  /* amplifier la lumière en fonction du son */
  for(i = 0; i < 3; ++i) {
    double a = pow(_moyenne_son, 2.5);
    amplified_amb_light[i]  = blanc[i] * (1.0 + 0.5 * a); 
    amplified_diff_light[i] = jaune_clair[i] * (1.0 + 1.5 * a); 
    amplified_spec_light[i] = blanc[i] * (1.0 + 5.0 * a); 
  }
  glUniform4fv(glGetUniformLocation(_pId, "light_ambient_color"), 1, amplified_amb_light);
  glUniform4fv(glGetUniformLocation(_pId, "light_diffuse_color"), 1, amplified_diff_light);
  glUniform4fv(glGetUniformLocation(_pId, "light_specular_color"), 1, amplified_spec_light);
  glUniform4fv(glGetUniformLocation(_pId, "light_position"), 1, position_lumiere);

  /***** On commence par la sphère *****/
  /* mettre la matrice identité (celle qui ne change rien) dans la matrice courante */
  gl4duLoadIdentityf();
  /* composer (multiplication à droite) avec un scale lié à l'intensité du son */
  {
    double s = 1.0 + 0.5 * pow(_moyenne_son, 2.0);
    gl4duScalef(s, s, s);
  }
  /* composer (multiplication à droite) avec une translation vers le
     haut <0, 1.5, 0> */
  gl4duTranslatef(0, 1.5f, 0);  
  /* composer (multiplication à droite) avec une rotation d'angle a et
     d'axe (autour de l'axe) <0, 1, 0> */
  gl4duRotatef(a / 5.0f, 0, 1, 0);
  /* envoyer les matrice GL4D au programme GPU OpenGL (en cours) */
  gl4duSendMatrices();
  glUniform4fv(glGetUniformLocation(_pId, "surface_ambient_color"), 1, rouge);
  glUniform4fv(glGetUniformLocation(_pId, "surface_specular_color"), 1, rouge);
  glUniform4fv(glGetUniformLocation(_pId, "surface_diffuse_color"), 1, rouge);
  /* demander le dessin d'un objet GL4D */
  gl4dgDraw(_cone);


  /***** On continue avec le plan *****/
  /* mettre la matrice identité (celle qui ne change rien) dans la matrice courante */
  gl4duLoadIdentityf();
  /* composer (multiplication à droite) avec une rotation d'angle -90 et
     d'axe (autour de l'axe) <1, 0, 0> pour coucher le plan */
  gl4duRotatef(-90, 1, 0, 0);
  /* composer (multiplication à droite) avec un scale x5 <15, 15, 15> */
  gl4duScalef(15, 15, 15);  
  /* envoyer les matrice GL4D au programme GPU OpenGL (en cours) */
  gl4duSendMatrices();
  glUniform4fv(glGetUniformLocation(_pId, "surface_ambient_color"), 1, blanc);
  glUniform4fv(glGetUniformLocation(_pId, "surface_diffuse_color"), 1, vert_tres_clair);
  glUniform4fv(glGetUniformLocation(_pId, "surface_specular_color"), 1, blanc);

  /* activer la l'unité 1 pour y stocker une texture de normal map */
  glActiveTexture(GL_TEXTURE1);
  /* binder la texture _texId[2] (normal map de brick) pour l'utiliser sur l'unité 1 */
  glBindTexture(GL_TEXTURE_2D, _texId[2]);
  /* informer le programme GPU _pId que ma texture est celle posée à l'unité 1 */
  glUniform1i(glGetUniformLocation(_pId, "my_nm_texture"), 1 /* le 1 correspond à GL_TEXTURE1 */);
  /* dire que je utilise une normal map */
  glUniform1i(glGetUniformLocation(_pId, "use_nm_texture"), GL_TRUE);
  /* activer la l'unité 0 pour y stocker une texture */
  glActiveTexture(GL_TEXTURE0);
  /* binder la texture _texId[1] (brick) pour l'utiliser sur l'unité 0 */
  glBindTexture(GL_TEXTURE_2D, _texId[1]);
  /* informer le programme GPU _pId que ma texture est celle posée à l'unité 0 */
  glUniform1i(glGetUniformLocation(_pId, "my_texture"), 0 /* le 0 correspond à GL_TEXTURE0 */);
  /* informer le programme GPU _pId que pour cette surface je souhaite utiliser la texture */
  glUniform1i(glGetUniformLocation(_pId, "use_texture"), GL_TRUE);
  /* mettre en place le facteur de multiplication de texture (répétition, voir le vertex shader) */
  glUniform1f(glGetUniformLocation(_pId, "mult_tex_coord"), 20.0f);

  /* demander le dessin d'un objet GL4D */
  gl4dgDraw(_plan);

  /* après mon draw informer le programme GPU _pId que je ne souhaite plus utiliser de texture */
  glUniform1i(glGetUniformLocation(_pId, "use_texture"), GL_FALSE);
  /* remettre le facteur de multiplication de texture (répétition, voir le vertex shader) à 1 */
  glUniform1f(glGetUniformLocation(_pId, "mult_tex_coord"), 1.0f);
  /* dire que je n'utilise plus de normal map */
  glUniform1i(glGetUniformLocation(_pId, "use_nm_texture"), GL_FALSE);
  /* dé-binder ma texture pour la désaffecter de l'unité 1 */
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, 0);
  /* dé-binder ma texture pour la désaffecter de l'unité 0 */
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);

  /***** On fini avec le cube *****/
  /* mettre la matrice identité (celle qui ne change rien) dans la matrice courante */
  gl4duLoadIdentityf();
  /* composer (multiplication à droite) avec une translation vers le
     haut <0, 1.5, 0> */
  gl4duTranslatef(0, 1.5f, 0);  
  /* composer (multiplication à droite) avec une rotation d'angle -90 et
     d'axe (autour de l'axe) <1, 0, 0> pour coucher le plan */
  gl4duRotatef(a / 2.0f, 0, 1, 0);
  /* composer (multiplication à droite) avec une translation vers la
     droite <3, 0, 0> */
  gl4duTranslatef(3, 0, 0);  
  gl4duRotatef(-a, 1, 0, 0);
  /* envoyer les matrice GL4D au programme GPU OpenGL (en cours) */
  gl4duSendMatrices();
  glUniform4fv(glGetUniformLocation(_pId, "surface_ambient_color"), 1, bleu);
  glUniform4fv(glGetUniformLocation(_pId, "surface_diffuse_color"), 1, bleu);
  glUniform4fv(glGetUniformLocation(_pId, "surface_specular_color"), 1, blanc);

  /* activer la l'unité 0 pour y stocker une texture */
  glActiveTexture(GL_TEXTURE0);
  /* binder la texture _texId[0] pour l'utiliser sur l'unité 0 */
  glBindTexture(GL_TEXTURE_2D, _texId[0]);
  /* informer le programme GPU _pId que ma texture est celle posée à l'unité 0 */
  glUniform1i(glGetUniformLocation(_pId, "my_texture"), 0 /* le 0 correspond à GL_TEXTURE0 */);
  /* informer le programme GPU _pId que pour cette surface je souhaite utiliser la texture */
  glUniform1i(glGetUniformLocation(_pId, "use_texture"), GL_TRUE);

  /* demander le dessin d'un objet GL4D */
  gl4dgDraw(_sphere);

  /* après mon draw informer le programme GPU _pId que je ne souhaite plus utiliser de texture */
  glUniform1i(glGetUniformLocation(_pId, "use_texture"), GL_FALSE);
  /* dé-binder ma texture pour la désaffecter de l'unité 0 */
  glBindTexture(GL_TEXTURE_2D, 0);

  /* n'utiliser aucun programme GPU (pas nécessaire) */
  glUseProgram(0);
  /* augmenter l'ange a de 1 */
  a += 60.0 * inter_frames_dt();
}

/* appelée lors du exit */
void quit(void) {
  /* arrêt de la musique et libération des ressources */
  if(_mmusic) {
    if(Mix_PlayingMusic())
      Mix_HaltMusic();
    Mix_FreeMusic(_mmusic);
    _mmusic = NULL;
  }
  Mix_CloseAudio();
  Mix_Quit();
  /* libérer les textures générées côté OpenGL/GPU */
  if(_texId[0]) {
    glDeleteTextures(3, _texId);
    _texId[0] = 0;
  }
  /* nettoyer (libérer) tout objet créé avec GL4D */
  gl4duClean(GL4DU_ALL);
}

