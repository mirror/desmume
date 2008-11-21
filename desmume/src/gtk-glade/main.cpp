/* main.c - this file is part of DeSmuME
 *
 * Copyright (C) 2007 Damien Nozay (damdoum)
 * Copyright (C) 2007 Pascal Giard (evilynux)
 * Author: damdoum at users.sourceforge.net
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

#include "callbacks.h"
#include "callbacks_IO.h"
#include "dTools/callbacks_dtools.h"
#include "globals.h"
#include "keyval_names.h"

#ifdef GDB_STUB
#include "../gdbstub.h"
#endif

#ifdef GTKGLEXT_AVAILABLE
#include <gtk/gtkgl.h>
#include "../OGLRender.h"
#include "gdk_3Demu.h"
#endif

/*
 * The frame limiter semaphore
 */
SDL_sem *glade_fps_limiter_semaphore;
int glade_fps_limiter_disabled = 0;

GtkWidget * pWindow;
GtkWidget * pDrawingArea, * pDrawingArea2;
GladeXML  * xml, * xml_tools;

SoundInterface_struct *SNDCoreList[] = {
&SNDDummy,
&SNDFile,
&SNDSDL,
NULL
};

GPU3DInterface *core3DList[] = {
&gpu3DNull
#ifdef GTKGLEXT_AVAILABLE
  ,
  &gpu3Dgl
#endif
};


/*
 *
 * Command line handling
 *
 */
struct configured_features {
  int software_colour_convert;
  int disable_3d;
  int disable_limiter;

  u16 arm9_gdb_port;
  u16 arm7_gdb_port;

  int firmware_language;

  const char *nds_file;
};

static void
init_configured_features( struct configured_features *config) {
  config->arm9_gdb_port = 0;
  config->arm7_gdb_port = 0;

  config->software_colour_convert = 0;

  config->disable_3d = 0;

  config->disable_limiter = 0;

  config->nds_file = NULL;

  /* use the default language */
  config->firmware_language = -1;
}

static int
fill_configured_features( struct configured_features *config,
                          int argc, char ** argv) {
  int good_args = 1;
  int print_usage = 0;
  int i;

  for ( i = 1; i < argc && good_args; i++) {
    if ( strcmp( argv[i], "--help") == 0) {
      g_print( _("USAGE: %s [OPTIONS] [nds-file]\n"), argv[0]);
      g_print( _("OPTIONS:\n"));
#ifdef GTKGLEXT_AVAILABLE
      g_print( _("\
   --soft-convert      Use software colour conversion during OpenGL\n\
                       screen rendering. May produce better or worse\n\
                       frame rates depending on hardware.\n\
   \n\
   --disable-3d        Disables the 3D emulation\n\n"));
#endif
      g_print( _("\
   --disable-limiter   Disables the 60 fps limiter\n\
   \n\
   --fwlang=LANG       Set the language in the firmware, LANG as follows:\n\
                         0 = Japanese\n\
                         1 = English\n\
                         2 = French\n\
                         3 = German\n\
                         4 = Italian\n\
                         5 = Spanish\n\n"));
#ifdef GDB_STUB
      g_print( _("\
   --arm9gdb=PORT_NUM  Enable the ARM9 GDB stub on the given port\n\
   --arm7gdb=PORT_NUM  Enable the ARM7 GDB stub on the given port\n\n"));
#endif
      g_print( _("\
   --help              Display this message\n"));
      //g_print("   --sticky            Enable sticky keys and stylus\n");
      good_args = 0;
    }
#ifdef GTKGLEXT_AVAILABLE
    else if ( strcmp( argv[i], "--soft-convert") == 0) {
      config->software_colour_convert = 1;
    }
    else if ( strcmp( argv[i], "--disable-3d") == 0) {
      config->disable_3d = 1;
    }
#endif
    else if ( strncmp( argv[i], "--fwlang=", 9) == 0) {
      char *end_char;
      int lang = strtoul( &argv[i][9], &end_char, 10);

      if ( lang >= 0 && lang <= 5) {
        config->firmware_language = lang;
      }
      else {
        g_printerr( _("Firmware language must be set to a value from 0 to 5.\n"));
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
        g_print( _("ARM9 GDB stub port must be in the range 1 to 65535\n"));
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
        g_print( _("ARM7 GDB stub port must be in the range 1 to 65535\n"));
        good_args = 0;
      }
    }
#endif
    else if ( strcmp( argv[i], "--disable-limiter") == 0) {
      config->disable_limiter = 1;
    }
    else {
      if ( config->nds_file == NULL) {
        config->nds_file = argv[i];
      }
      else {
        g_print( _("NDS file (\"%s\") already set\n"), config->nds_file);
        good_args = 0;
      }
    }
  }

  if ( good_args) {
    /*
     * check if the configured features are consistant
     */
  }

  if ( print_usage) {
    g_print( _("USAGE: %s [options] [nds-file]\n"), argv[0]);
    g_print( _("USAGE: %s --help    - for help\n"), argv[0]);
  }

  return good_args;
}



/* ***** ***** TOOLS ***** ***** */

GList * tools_to_update = NULL;

// register tool
void register_Tool(VoidFunPtr fun) {
	tools_to_update = g_list_append(tools_to_update, (void *) fun);
}
void unregister_Tool(VoidFunPtr fun) {
	if (tools_to_update == NULL) return;
	tools_to_update = g_list_remove(tools_to_update, (void *) fun);
}

static void notify_Tool (VoidFunPtr fun, gpointer func_data) {
	fun();
}

void notify_Tools() {
	g_list_foreach(tools_to_update, (GFunc)notify_Tool, NULL);
}

/* Return the glade directory. 
   Note: See configure.ac for the value of GLADEUI_UNINSTALLED_DIR. */
gchar * get_ui_file (const char *filename)
{
	gchar *path;

	/* looking in uninstalled (aka building) dir first */
	path = g_build_filename (GLADEUI_UNINSTALLED_DIR, filename, NULL);
	if (g_file_test (path, G_FILE_TEST_IS_REGULAR)) return path;
	g_free (path);
	
	/* looking in installed dir */
	path = g_build_filename (DATADIR, filename, NULL);
	if (g_file_test (path, G_FILE_TEST_IS_REGULAR)) return path;
	g_free (path);
	
	/* not found */
	return NULL;
}


/* ***** ***** CONFIG FILE ***** ***** */
char * CONFIG_FILE;

static int Read_ConfigFile()
{
	int i, tmp;
	GKeyFile * keyfile = g_key_file_new();
	GError * error = NULL;
	
	load_default_config();
	
	g_key_file_load_from_file(keyfile, CONFIG_FILE, G_KEY_FILE_NONE, 0);

	/* Load keypad keys */
	for(i = 0; i < NB_KEYS; i++)
	{
		tmp = g_key_file_get_integer(keyfile, "KEYS", key_names[i], &error);
		if (error != NULL) {
                  g_error_free(error);
                  error = NULL;
		} else {
                  keyboard_cfg[i] = tmp;
		}
	}
		
	/* Load joypad keys */
	for(i = 0; i < NB_KEYS; i++)
	{
		tmp = g_key_file_get_integer(keyfile, "JOYKEYS", key_names[i], &error);
		if (error != NULL) {
                  g_error_free(error);
                  error = NULL;
		} else {
                  joypad_cfg[i] = tmp;
		}
	}

	g_key_file_free(keyfile);
		
	return 0;
}

static int Write_ConfigFile()
{
	int i;
	GKeyFile * keyfile;
	gchar *contents;
	
	keyfile = g_key_file_new();
	
	for(i = 0; i < NB_KEYS; i++)
	{
		g_key_file_set_integer(keyfile, "KEYS", key_names[i], keyboard_cfg[i]);
		g_key_file_set_integer(keyfile, "JOYKEYS", key_names[i], joypad_cfg[i]);
	}

	contents = g_key_file_to_data(keyfile, 0, 0);
	g_file_set_contents(CONFIG_FILE, contents, -1, 0);
	g_free(contents);

	g_key_file_free(keyfile);
	
	return 0;
}


/*
 * The thread handling functions needed by the GDB stub code.
 */
#ifdef GDB_STUB
void *
createThread_gdb( void (*thread_function)( void *data),
                  void *thread_data) {
  GThread *new_thread = g_thread_create( (GThreadFunc)thread_function,
                                         thread_data,
                                         TRUE,
                                         NULL);

  return new_thread;
}

void
joinThread_gdb( void *thread_handle) {
  g_thread_join((GThread *) thread_handle);
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
glade_fps_limiter_fn( Uint32 interval, void *param) {
  SDL_sem *sdl_semaphore = (SDL_sem *)param;

  /* signal the semaphore if it is getting low */
  if ( SDL_SemValue( sdl_semaphore) < 4) {
    SDL_SemPost( sdl_semaphore);
  }

  return interval;
}

/* ***** ***** MAIN ***** ***** */

static int
common_gtk_glade_main( struct configured_features *my_config) {
	SDL_TimerID limiter_timer;
#ifdef GDB_STUB
        gdbstub_handle_t arm9_gdb_stub;
        gdbstub_handle_t arm7_gdb_stub;
#endif
        struct armcpu_memory_iface *arm9_memio = &arm9_base_memory_iface;
        struct armcpu_memory_iface *arm7_memio = &arm7_base_memory_iface;
        struct armcpu_ctrl_iface *arm9_ctrl_iface;
        struct armcpu_ctrl_iface *arm7_ctrl_iface;
        /* the firmware settings */
        struct NDS_fw_config_data fw_config;
	gchar *uifile;

        /* default the firmware settings, they may get changed later */
        NDS_FillDefaultFirmwareConfigData( &fw_config);

        /* use any language set on the command line */
        if ( my_config->firmware_language != -1) {
          fw_config.language = my_config->firmware_language;
        }

#ifdef GTKGLEXT_AVAILABLE
// check if you have GTHREAD when running configure script
	//g_thread_init(NULL);
	register_gl_fun(my_gl_Begin,my_gl_End);
#endif
	init_keyvals();

#ifdef GDB_STUB
        if ( my_config->arm9_gdb_port != 0) {
          arm9_gdb_stub = createStub_gdb( my_config->arm9_gdb_port,
                                          &arm9_memio,
                                          &arm9_base_memory_iface);

          if ( arm9_gdb_stub == NULL) {
            g_print( _("Failed to create ARM9 gdbstub on port %d\n"),
                     my_config->arm9_gdb_port);
            return -1;
          }
        }
        if ( my_config->arm7_gdb_port != 0) {
          arm7_gdb_stub = createStub_gdb( my_config->arm7_gdb_port,
                                          &arm7_memio,
                                          &arm7_base_memory_iface);

          if ( arm7_gdb_stub == NULL) {
            g_print( _("Failed to create ARM7 gdbstub on port %d\n"),
                     my_config->arm7_gdb_port);
            return -1;
          }
        }
#endif

	if(SDL_Init( SDL_INIT_TIMER | SDL_INIT_VIDEO) == -1)
          {
            fprintf(stderr, _("Error trying to initialize SDL: %s\n"),
                    SDL_GetError());
            return 1;
          }

	desmume_init( arm9_memio, &arm9_ctrl_iface,
                      arm7_memio, &arm7_ctrl_iface);


        /* Create the dummy firmware */
        NDS_CreateDummyFirmware( &fw_config);

        /*
         * Activate the GDB stubs
         * This has to come after the NDS_Init (called in desmume_init)
         * where the cpus are set up.
         */
#ifdef GDB_STUB
        if ( my_config->arm9_gdb_port != 0) {
          activateStub_gdb( arm9_gdb_stub, arm9_ctrl_iface);
        }
        if ( my_config->arm7_gdb_port != 0) {
          activateStub_gdb( arm7_gdb_stub, arm7_ctrl_iface);
        }
#endif

        /* Initialize joysticks */
        if(!init_joy()) return 1;

	CONFIG_FILE = g_build_filename(g_get_home_dir(), ".desmume.ini", NULL);
	Read_ConfigFile();

	/* load the interface */
	uifile        = get_ui_file("DeSmuMe.glade");
	xml           = glade_xml_new(uifile, NULL, NULL);
	g_free (uifile);
	uifile        = get_ui_file("DeSmuMe_Dtools.glade");
	xml_tools     = glade_xml_new(uifile, NULL, NULL);
	g_free (uifile);
	pWindow       = glade_xml_get_widget(xml, "wMainW");
	pDrawingArea  = glade_xml_get_widget(xml, "wDraw_Main");
	pDrawingArea2 = glade_xml_get_widget(xml, "wDraw_Sub");

	/* connect the signals in the interface */
	glade_xml_signal_autoconnect_StringObject(xml);
	glade_xml_signal_autoconnect_StringObject(xml_tools);

	init_GL_capabilities( my_config->software_colour_convert);

	/* check command line file */
	if( my_config->nds_file) {
		if(desmume_open( my_config->nds_file) >= 0)	{
			desmume_resume();
			enable_rom_features();
		} else {
			GtkWidget *pDialog = gtk_message_dialog_new(GTK_WINDOW(pWindow),
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_INFO,
					GTK_BUTTONS_OK,
					_("Unable to load :\n%s"), my_config->nds_file);
			gtk_dialog_run(GTK_DIALOG(pDialog));
			gtk_widget_destroy(pDialog);
		}
	}

        gtk_widget_show(pDrawingArea);
        gtk_widget_show(pDrawingArea2);

        {
          int use_null_3d = my_config->disable_3d;

#ifdef GTKGLEXT_AVAILABLE
          if ( !use_null_3d) {
            /* setup the gdk 3D emulation */
            if ( init_opengl_gdk_3Demu()) {
              NDS_3D_SetDriver(1);

              if (!gpu3D->NDS_3D_Init()) {
                fprintf( stderr, _("Failed to initialise openGL 3D emulation; "
                         "removing 3D support\n"));
                use_null_3d = 1;
              }
            }
            else {
              fprintf( stderr, _("Failed to setup openGL 3D emulation; "
                       "removing 3D support\n"));
              use_null_3d = 1;
            }
          }
#endif
          if ( use_null_3d) {
            NDS_3D_SetDriver ( 0);
            gpu3D->NDS_3D_Init();
          }
        }

//	on_menu_tileview_activate(NULL,NULL);

        /* setup the frame limiter and indicate if it is disabled */
        glade_fps_limiter_disabled = my_config->disable_limiter;

        if ( !glade_fps_limiter_disabled) {
          /* create the semaphore used for fps limiting */
          glade_fps_limiter_semaphore = SDL_CreateSemaphore( 1);

          /* start a SDL timer for every FPS_LIMITER_FRAME_PERIOD
           * frames to keep us at 60 fps */
          limiter_timer = SDL_AddTimer( 16 * FPS_LIMITER_FRAME_PERIOD,
                                        glade_fps_limiter_fn,
                                        glade_fps_limiter_semaphore);
          if ( limiter_timer == NULL) {
            fprintf( stderr, _("Error trying to start FPS limiter timer: %s\n"),
                     SDL_GetError());
            SDL_DestroySemaphore( glade_fps_limiter_semaphore);
            glade_fps_limiter_disabled = 1;
          }
        }

	/* start event loop */
	gtk_main();
	desmume_free();

        if ( !glade_fps_limiter_disabled) {
          /* tidy up the FPS limiter timer and semaphore */
          SDL_RemoveTimer( limiter_timer);
          SDL_DestroySemaphore( glade_fps_limiter_semaphore);
        }

        /* Unload joystick */
        uninit_joy();

	SDL_Quit();
	Write_ConfigFile();
	return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  struct configured_features my_config;

  // Localization
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  init_configured_features( &my_config);

  if (!g_thread_supported())
    g_thread_init( NULL);

  gtk_init(&argc, &argv);

#ifdef GTKGLEXT_AVAILABLE
  gtk_gl_init( &argc, &argv);
#endif

  if ( !fill_configured_features( &my_config, argc, argv)) {
    exit(0);
  }

  return common_gtk_glade_main( &my_config);
}


#ifdef WIN32
int WinMain ( HINSTANCE hThisInstance, HINSTANCE hPrevInstance,
              LPSTR lpszArgument, int nFunsterStil)
{
  int argc = 0;
  char *argv[] = NULL;

  /*
   * FIXME:
   * Emulate the argc and argv main parameters. Could do this using
   * CommandLineToArgvW and then convert the wide chars to thin chars.
   * Or parse the wide chars directly and call common_gtk_glade_main with a
   * filled configuration structure.
   */
  main( argc, argv);
}
#endif

