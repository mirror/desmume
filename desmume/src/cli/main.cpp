/* main.c - this file is part of DeSmuME
 *
 * Copyright (C) 2006,2007 DeSmuME Team
 * Copyright (C) 2007 Pascal Giard (evilynux)
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>


#ifndef VERSION
#define VERSION "Unknown version"
#endif

/*
 * FIXME: Not sure how to detect OpenGL in a platform portable way.
 */
#ifdef HAVE_GL_GL_H
#define INCLUDE_OPENGL_2D 1
#endif

#ifdef INCLUDE_OPENGL_2D
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#ifndef CLI_UI
#define CLI_UI
#endif

#include "../MMU.h"
#include "../NDSSystem.h"
#include "../cflash.h"
#include "../debug.h"
#include "../sndsdl.h"
#include "../ctrlssdl.h"
#include "../render3D.h"
#ifdef GDB_STUB
#include "../gdbstub.h"
#endif

volatile BOOL execute = FALSE;

static float nds_screen_size_ratio = 1.0f;

#define DISPLAY_FPS 1

#ifdef DISPLAY_FPS
#define NUM_FRAMES_TO_TIME 60
#endif


#define FPS_LIMITER_FRAME_PERIOD 8

static SDL_Surface * surface;

/* Flags to pass to SDL_SetVideoMode */
static int sdl_videoFlags = 0;

SoundInterface_struct *SNDCoreList[] = {
  &SNDDummy,
  &SNDFile,
  &SNDSDL,
  NULL
};

GPU3DInterface *core3DList[] = {
&gpu3DNull
};


/* Our keyboard config is different because of the directional keys */
const u16 cli_kb_cfg[NB_KEYS] =
  { SDLK_c,         // A
    SDLK_x,         // B
    SDLK_BACKSPACE, // select
    SDLK_RETURN,    // start
    SDLK_RIGHT,     // Right
    SDLK_LEFT,      // Left
    SDLK_UP,        // Up
    SDLK_DOWN,      // Down
    SDLK_e,         // R
    SDLK_w,         // L
    SDLK_d,         // X
    SDLK_s,         // Y
    SDLK_p,         // DEBUG
    SDLK_o          // BOOST
  };

struct my_config {
  u16 arm9_gdb_port;
  u16 arm7_gdb_port;

  int disable_sound;

#ifdef INCLUDE_OPENGL_2D
  int opengl_2d;
  int soft_colour_convert;
#endif
  int disable_limiter;

  int firmware_language;

  const char *nds_file;
  const char *cflash_disk_image_file;
};

static void
init_config( struct my_config *config) {
  config->arm9_gdb_port = 0;
  config->arm7_gdb_port = 0;

  config->disable_sound = 0;

  config->disable_limiter = 0;

  config->nds_file = NULL;

  config->cflash_disk_image_file = NULL;

#ifdef INCLUDE_OPENGL_2D
  config->opengl_2d = 0;
  config->soft_colour_convert = 0;
#endif

  /* use the default language */
  config->firmware_language = -1;
}


static int
fill_config( struct my_config *config,
             int argc, char ** argv) {
  int good_args = 1;
  int print_usage = 0;
  int i;

  for ( i = 1; i < argc && good_args; i++) {
    if ( strcmp( argv[i], "--help") == 0) {
      printf( "USAGE: %s <nds-file>\n", argv[0]);
      printf( "OPTIONS:\n");
      printf( "   --disable-sound         Disables the sound emulation\n");
      printf( "   --disable-limiter   Disables the 60 fps limiter\n");
#ifdef INCLUDE_OPENGL_2D
      printf( "   --opengl-2d         Enables using OpenGL for screen rendering\n");
      printf( "    --soft-convert     Use software colour conversion during OpenGL\n");
      printf( "                       screen rendering. May produce better or worse\n");
      printf( "                       frame rates depending on hardware.\n");
#endif
      printf( "\n");
      printf( "   --fwlang=LANG       Set the language in the firmware, LANG as follows:\n");
      printf( "                         0 = Japanese\n");
      printf( "                         1 = English\n");
      printf( "                         2 = French\n");
      printf( "                         3 = German\n");
      printf( "                         4 = Italian\n");
      printf( "                         5 = Spanish\n");
      printf( "\n");
#ifdef GDB_STUB
      printf( "   --arm9gdb=PORT_NUM      Enable the ARM9 GDB stub on the given port\n");
      printf( "   --arm7gdb=PORT_NUM      Enable the ARM7 GDB stub on the given port\n");
#endif
      //printf( "   --sticky                Enable sticky keys and stylus\n");
      printf( "\n");
      printf( "   --cflash=PATH_TO_DISK_IMAGE\n");
      printf( "                       Enable disk image GBAMP compact flash emulation\n");
      printf( "\n");
      printf( "   --help                  Display this message\n");
      printf( "   --version               Display the version\n");
      good_args = 0;
    }
    else if ( strcmp( argv[i], "--version") == 0) {
      printf( "%s\n", VERSION);
      good_args = 0;
    }
    else if ( strcmp( argv[i], "--disable-sound") == 0) {
      config->disable_sound = 1;
    }
#ifdef INCLUDE_OPENGL_2D
    else if ( strcmp( argv[i], "--opengl-2d") == 0) {
      config->opengl_2d = 1;
    }
    else if ( strcmp( argv[i], "--soft-convert") == 0) {
      config->soft_colour_convert = 1;
    }
#endif
    else if ( strcmp( argv[i], "--disable-limiter") == 0) {
      config->disable_limiter = 1;
    }
    else if ( strncmp( argv[i], "--fwlang=", 9) == 0) {
      char *end_char;
      int lang = strtoul( &argv[i][9], &end_char, 10);

      if ( lang >= 0 && lang <= 5) {
        config->firmware_language = lang;
      }
      else {
        fprintf( stderr, "Firmware language must be set to a value from 0 to 5.\n");
        good_args = 0;
      }
    }
#ifdef GDB_STUB
    else if ( strncmp( argv[i], "--arm9gdb=", 10) == 0) {
      char *end_char;
      unsigned long port_num = strtoul( &argv[i][10], &end_char, 10);

      if ( port_num > 0 && port_num < 65536) {
        config->arm9_gdb_port = port_num;
      }
      else {
        fprintf( stderr, "ARM9 GDB stub port must be in the range 1 to 65535\n");
        good_args = 0;
      }
    }
    else if ( strncmp( argv[i], "--arm7gdb=", 10) == 0) {
      char *end_char;
      unsigned long port_num = strtoul( &argv[i][10], &end_char, 10);

      if ( port_num > 0 && port_num < 65536) {
        config->arm7_gdb_port = port_num;
      }
      else {
        fprintf( stderr, "ARM7 GDB stub port must be in the range 1 to 65535\n");
        good_args = 0;
      }
    }
#endif
    else if ( strncmp( argv[i], "--cflash=", 9) == 0) {
      if ( config->cflash_disk_image_file == NULL) {
        config->cflash_disk_image_file = &argv[i][9];
      }
      else {
        fprintf( stderr, "CFlash disk image file (\"%s\") already set\n",
                 config->cflash_disk_image_file);
        good_args = 0;
      }
    }
    else {
      if ( config->nds_file == NULL) {
        config->nds_file = argv[i];
      }
      else {
        fprintf( stderr, "NDS file (\"%s\") already set\n", config->nds_file);
        good_args = 0;
      }
    }
  }

  if ( good_args) {
    if ( config->nds_file == NULL) {
      print_usage = 1;
      good_args = 0;
    }
  }

  if ( print_usage) {
    fprintf( stderr, "USAGE: %s <nds-file>\n", argv[0]);
  }

  return good_args;
}

/*
 * The thread handling functions needed by the GDB stub code.
 */
#ifdef GDB_STUB
void *
createThread_gdb( void (*thread_function)( void *data),
                  void *thread_data) {
  SDL_Thread *new_thread = SDL_CreateThread( (int (*)(void *data))thread_function,
                                             thread_data);

  return new_thread;
}

void
joinThread_gdb( void *thread_handle) {
  int ignore;
  SDL_WaitThread( (SDL_Thread*)thread_handle, &ignore);
}
#endif



/** 
 * A SDL timer callback function. Signals the supplied SDL semaphore
 * if its value is small.
 * 
 * @param interval The interval since the last call (in ms)
 * @param param The pointer to the semaphore.
 * 
 * @return The interval to the next call (required by SDL)
 */
static Uint32
fps_limiter_fn( Uint32 interval, void *param) {
  SDL_sem *sdl_semaphore = (SDL_sem *)param;

  /* signal the semaphore if it is getting low */
  if ( SDL_SemValue( sdl_semaphore) < 4) {
    SDL_SemPost( sdl_semaphore);
  }

  return interval;
}


#ifdef INCLUDE_OPENGL_2D
/* initialization openGL function */
static int
initGL( GLuint *screen_texture) {
  GLenum errCode;
  int init_good = 1;
  int i;
  u16 blank_texture[256 * 512];

  for ( i = 0; i < 256 * 512; i++) {
    blank_texture[i] = 0x001f;
  }


  /* Enable Texture Mapping */
  glEnable( GL_TEXTURE_2D );

  /* Set the background black */
  glClearColor( 0.0f, 0.0f, 0.0f, 0.5f );

  /* Depth buffer setup */
  glClearDepth( 1.0f );

  /* Enables Depth Testing */
  glEnable( GL_DEPTH_TEST );

  /* The Type Of Depth Test To Do */
  glDepthFunc( GL_LEQUAL );

  /* Create The Texture */
  glGenTextures( 1, &screen_texture[0]);

  glBindTexture( GL_TEXTURE_2D, screen_texture[0]);

  /* Generate The Texture */
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, 256, 512,
                0, GL_RGBA,
                GL_UNSIGNED_SHORT_1_5_5_5_REV,
                blank_texture);

  /* Linear Filtering */
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  if ((errCode = glGetError()) != GL_NO_ERROR) {
    const GLubyte *errString;

    errString = gluErrorString(errCode);
    fprintf( stderr, "Failed to init GL: %s\n", errString);

    init_good = 0;
  }

  return init_good;
}

static void
resizeWindow( u16 width, u16 height) {
  int comp_width = 3 * width;
  int comp_height = 2 * height;
  int use_width = 1;
  GLenum errCode;

  /* Height / width ration */
  GLfloat ratio;

  if ( comp_width > comp_height) {
    use_width = 0;
  }
 
  /* Protect against a divide by zero */
  if ( height == 0 )
    height = 1;
  if ( width == 0)
    width = 1;

  ratio = ( GLfloat )width / ( GLfloat )height;

  /* Setup our viewport. */
  glViewport( 0, 0, ( GLint )width, ( GLint )height );

  /*
   * change to the projection matrix and set
   * our viewing volume.
   */
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity( );

  {
    double left;
    double right;
    double bottom;
    double top;
    double other_dimen;

    if ( use_width) {
      left = 0.0;
      right = 256.0;

      nds_screen_size_ratio = 256.0 / (double)width;

      other_dimen = (double)width * 3.0 / 2.0;

      top = 0.0;
      bottom = 384.0 * ((double)height / other_dimen);
    }
    else {
      top = 0.0;
      bottom = 384.0;

      nds_screen_size_ratio = 384.0 / (double)height;

      other_dimen = (double)height * 2.0 / 3.0;

      left = 0.0;
      right = 256.0 * ((double)width / other_dimen);
    }

    /*
    printf("%d,%d\n", width, height);
    printf("l %lf, r %lf, t %lf, b %lf, other dimen %lf\n",
           left, right, top, bottom, other_dimen);
    */

    /* get the area (0,0) to (256,384) into the middle of the viewport */
    gluOrtho2D( left, right, bottom, top);
  }

  /* Make sure we're chaning the model view and not the projection */
  glMatrixMode( GL_MODELVIEW );

  /* Reset The View */
  glLoadIdentity( );

  if ((errCode = glGetError()) != GL_NO_ERROR) {
    const GLubyte *errString;

    errString = gluErrorString(errCode);
    fprintf( stderr, "GL resize failed: %s\n", errString);
  }

  surface = SDL_SetVideoMode( width, height, 32,
                              sdl_videoFlags );
}


static void
opengl_Draw( GLuint *texture, int software_convert) {
  GLenum errCode;

  /* Clear The Screen And The Depth Buffer */
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  /* Move Into The Screen 5 Units */
  glLoadIdentity( );

  /* Select screen Texture */
  glBindTexture( GL_TEXTURE_2D, texture[0]);
  if ( software_convert) {
    int i;
    u8 converted[256 * 384 * 3];

    for ( i = 0; i < (256 * 384); i++) {
      converted[(i * 3) + 0] = ((*((u16 *)&GPU_screen[(i<<1)]) >> 0) & 0x1f) << 3;
      converted[(i * 3) + 1] = ((*((u16 *)&GPU_screen[(i<<1)]) >> 5) & 0x1f) << 3;
      converted[(i * 3) + 2] = ((*((u16 *)&GPU_screen[(i<<1)]) >> 10) & 0x1f) << 3;
    }

    glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 256, 384,
                     GL_RGB,
                     GL_UNSIGNED_BYTE,
                     converted);
  }
  else {
    glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 256, 384,
                     GL_RGBA,
                     GL_UNSIGNED_SHORT_1_5_5_5_REV,
                     &GPU_screen);
  }

  if ((errCode = glGetError()) != GL_NO_ERROR) {
    const GLubyte *errString;

    errString = gluErrorString(errCode);
    fprintf( stderr, "GL subimage failed: %s\n", errString);
  }


  /* Draw the screen as a textured quad */
  glBegin( GL_QUADS);
  glTexCoord2f( 0.0f, 0.0f ); glVertex3f( 0.0f,  0.0f, 0.0f );
  glTexCoord2f( 1.0f, 0.0f ); glVertex3f( 256.0f,  0.0f,  0.0f );
  glTexCoord2f( 1.0f, 0.75f ); glVertex3f( 256.0f,  384.0f,  0.0f );
  glTexCoord2f( 0.0f, 0.75f ); glVertex3f(  0.0f,  384.0f, 0.0f );
  glEnd( );

  if ((errCode = glGetError()) != GL_NO_ERROR) {
    const GLubyte *errString;

    errString = gluErrorString(errCode);
    fprintf( stderr, "GL draw failed: %s\n", errString);
  }

  /* Draw it to the screen */
  SDL_GL_SwapBuffers( );
}
#endif

static void
Draw( void) {
  SDL_Surface *rawImage;
	
  rawImage = SDL_CreateRGBSurfaceFrom((void*)&GPU_screen, 256, 384, 16, 512, 0x001F, 0x03E0, 0x7C00, 0);
  if(rawImage == NULL) return;
	
  SDL_BlitSurface(rawImage, 0, surface, 0);

  SDL_UpdateRect(surface, 0, 0, 0, 0);
  
  SDL_FreeSurface(rawImage);
  return;
}


int main(int argc, char ** argv) {
  static unsigned short keypad = 0;
  struct my_config my_config;
  u32 last_cycle = 0;
#ifdef GDB_STUB
  gdbstub_handle_t arm9_gdb_stub;
  gdbstub_handle_t arm7_gdb_stub;
  struct armcpu_memory_iface *arm9_memio = &arm9_base_memory_iface;
  struct armcpu_memory_iface *arm7_memio = &arm7_base_memory_iface;
  struct armcpu_ctrl_iface *arm9_ctrl_iface;
  struct armcpu_ctrl_iface *arm7_ctrl_iface;
#endif

  int limiter_frame_counter = 0;
  SDL_sem *fps_limiter_semaphore = NULL;
  SDL_TimerID limiter_timer = NULL;
  int sdl_quit = 0;

#ifdef DISPLAY_FPS
  u32 fps_timing = 0;
  u32 fps_frame_counter = 0;
  u32 fps_previous_time = 0;
  u32 fps_temp_time;
#endif

#ifdef INCLUDE_OPENGL_2D
  GLuint screen_texture[1];
#endif
  /* this holds some info about our display */
  const SDL_VideoInfo *videoInfo;

  /* the firmware settings */
  struct NDS_fw_config_data fw_config;

  /* default the firmware settings, they may get changed later */
  NDS_FillDefaultFirmwareConfigData( &fw_config);

  init_config( &my_config);

  if ( !fill_config( &my_config, argc, argv)) {
    exit(1);
  }

  /* use any language set on the command line */
  if ( my_config.firmware_language != -1) {
    fw_config.language = my_config.firmware_language;
  }

#ifdef GDB_STUB
  if ( my_config.arm9_gdb_port != 0) {
    arm9_gdb_stub = createStub_gdb( my_config.arm9_gdb_port,
                                    &arm9_memio,
                                    &arm9_direct_memory_iface);

    if ( arm9_gdb_stub == NULL) {
      fprintf( stderr, "Failed to create ARM9 gdbstub on port %d\n",
               my_config.arm9_gdb_port);
      exit( 1);
    }
  }
  if ( my_config.arm7_gdb_port != 0) {
    arm7_gdb_stub = createStub_gdb( my_config.arm7_gdb_port,
                                    &arm7_memio,
                                    &arm7_base_memory_iface);

    if ( arm7_gdb_stub == NULL) {
      fprintf( stderr, "Failed to create ARM7 gdbstub on port %d\n",
               my_config.arm7_gdb_port);
      exit( 1);
    }
  }
#endif

#ifdef GDB_STUB
  NDS_Init( arm9_memio, &arm9_ctrl_iface,
            arm7_memio, &arm7_ctrl_iface);
#else
        NDS_Init();
#endif

  /* Create the dummy firmware */
  NDS_CreateDummyFirmware( &fw_config);

  if ( !my_config.disable_sound) {
    SPU_ChangeSoundCore(SNDCORE_SDL, 735 * 4);
  }

  if (NDS_LoadROM( my_config.nds_file, MC_TYPE_AUTODETECT, 1, my_config.cflash_disk_image_file) < 0) {
    fprintf(stderr, "error while loading %s\n", my_config.nds_file);
    exit(-1);
  }

  /*
   * Activate the GDB stubs
   * This has to come after the NDS_Init where the cpus are set up.
   */
#ifdef GDB_STUB
  if ( my_config.arm9_gdb_port != 0) {
    activateStub_gdb( arm9_gdb_stub, arm9_ctrl_iface);
  }
  if ( my_config.arm7_gdb_port != 0) {
    activateStub_gdb( arm7_gdb_stub, arm7_ctrl_iface);
  }
#endif

  /*      // This has to get fixed yet
          strncpy(szRomPath, dirname(argv[1]), ARRAY_SIZE(szRomPath));
          cflash_close();
          cflash_init();
  */
  
  execute = TRUE;

  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1)
    {
      fprintf(stderr, "Error trying to initialize SDL: %s\n",
              SDL_GetError());
      return 1;
    }
  SDL_WM_SetCaption("Desmume SDL", NULL);

  /* Fetch the video info */
  videoInfo = SDL_GetVideoInfo( );
  if ( !videoInfo ) {
    fprintf( stderr, "Video query failed: %s\n", SDL_GetError( ) );
    exit( -1);
  }

  /* This checks if hardware blits can be done */
  if ( videoInfo->blit_hw )
    sdl_videoFlags |= SDL_HWACCEL;

#ifdef INCLUDE_OPENGL_2D
  if ( my_config.opengl_2d) {
    /* the flags to pass to SDL_SetVideoMode */
    sdl_videoFlags  = SDL_OPENGL;          /* Enable OpenGL in SDL */
    sdl_videoFlags |= SDL_GL_DOUBLEBUFFER; /* Enable double buffering */
    sdl_videoFlags |= SDL_HWPALETTE;       /* Store the palette in hardware */
    sdl_videoFlags |= SDL_RESIZABLE;       /* Enable window resizing */


    /* This checks to see if surfaces can be stored in memory */
    if ( videoInfo->hw_available )
      sdl_videoFlags |= SDL_HWSURFACE;
    else
      sdl_videoFlags |= SDL_SWSURFACE;


    /* Sets up OpenGL double buffering */
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    surface = SDL_SetVideoMode( 256, 192 * 2, 32,
                                sdl_videoFlags );

    /* Verify there is a surface */
    if ( !surface ) {
      fprintf( stderr, "Video mode set failed: %s\n", SDL_GetError( ) );
      exit( -1);
    }
    

    /* initialize OpenGL */
    if ( !initGL( screen_texture)) {
      fprintf( stderr, "Failed to init GL, fall back to software render\n");

      my_config.opengl_2d = 0;
    }
  }

  if ( !my_config.opengl_2d) {
#endif
    sdl_videoFlags |= SDL_SWSURFACE;
    surface = SDL_SetVideoMode(256, 384, 32, sdl_videoFlags);

    if ( !surface ) {
      fprintf( stderr, "Video mode set failed: %s\n", SDL_GetError( ) );
      exit( -1);
    }
#ifdef INCLUDE_OPENGL_2D
  }

  /* set the initial window size */
  if ( my_config.opengl_2d) {
    resizeWindow( 256, 192*2);
  }
#endif

  /* Initialize joysticks */
  if(!init_joy()) return 1;
  /* Load our own keyboard configuration */
  set_kb_keys(cli_kb_cfg);

  if ( !my_config.disable_limiter) {
    /* create the semaphore used for fps limiting */
    fps_limiter_semaphore = SDL_CreateSemaphore( 1);

    /* start a SDL timer for every FPS_LIMITER_FRAME_PERIOD frames to keep us at 60 fps */
    if ( fps_limiter_semaphore != NULL) {
      limiter_timer = SDL_AddTimer( 16 * FPS_LIMITER_FRAME_PERIOD,
                                  fps_limiter_fn, fps_limiter_semaphore);
    }

    if ( limiter_timer == NULL) {
      fprintf( stderr, "Error trying to start FPS limiter timer: %s\n",
               SDL_GetError());
      if ( fps_limiter_semaphore != NULL) {
        SDL_DestroySemaphore( fps_limiter_semaphore);
        fps_limiter_semaphore = NULL;
      }
      return 1;
    }
  }



  while(!sdl_quit) {
    /* Look for queued events and update keypad status */
#ifdef INCLUDE_OPENGL_2D
    sdl_quit = process_ctrls_events( &keypad, resizeWindow, nds_screen_size_ratio);
#else
    sdl_quit = process_ctrls_events( &keypad, NULL, nds_screen_size_ratio);
#endif
    /* Update mouse position and click */
    if(mouse.down) NDS_setTouchPos(mouse.x, mouse.y);
    if(mouse.click)
      { 
        NDS_releaseTouch();
        mouse.click = FALSE;
      }

    update_keypad(keypad);     /* Update keypad */
    last_cycle = NDS_exec<false>((560190 << 1) - last_cycle);
    SPU_Emulate_user();
	SPU_Emulate_core();

#ifdef INCLUDE_OPENGL_2D
    if ( my_config.opengl_2d) {
      opengl_Draw( screen_texture, my_config.soft_colour_convert);
    }
    else
#endif
      Draw();

    if ( !my_config.disable_limiter) {
      limiter_frame_counter += 1;
      if ( limiter_frame_counter >= FPS_LIMITER_FRAME_PERIOD) {
        limiter_frame_counter = 0;

        /* wait for the timer to expire */
        SDL_SemWait( fps_limiter_semaphore);
      }
    }

#ifdef DISPLAY_FPS
    fps_frame_counter += 1;
    fps_temp_time = SDL_GetTicks();
    fps_timing += fps_temp_time - fps_previous_time;
    fps_previous_time = fps_temp_time;

    if ( fps_frame_counter == NUM_FRAMES_TO_TIME) {
      char win_title[100];
      float fps = (float)fps_timing;
      fps /= NUM_FRAMES_TO_TIME * 1000.f;
      fps = 1.0f / fps;

      //printf("fps %f\n", fps);
      fps_frame_counter = 0;
      fps_timing = 0;

      sprintf( win_title, "Desmume %f", fps);

      SDL_WM_SetCaption( win_title, NULL);
    }
#endif
  }



  /* Unload joystick */
  uninit_joy();

  if ( !my_config.disable_limiter) {
    /* tidy up the FPS limiter timer and semaphore */
    SDL_RemoveTimer( limiter_timer);
    SDL_DestroySemaphore( fps_limiter_semaphore);
  }

  SDL_Quit();
  NDS_DeInit();

#ifdef GDB_STUB
  if ( my_config.arm9_gdb_port != 0) {
    destroyStub_gdb( arm9_gdb_stub);
  }
  if ( my_config.arm7_gdb_port != 0) {
    destroyStub_gdb( arm7_gdb_stub);
  }
#endif

  return 0;
}
