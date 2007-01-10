/* joysdl.c - this file is part of DeSmuME
 *
 * Copyright (C) 2007 Pascal Giard
 *
 * Author: Pascal Giard <evilynux@gmail.com>
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

#include "ctrlssdl.h"

/* Initialize joysticks */
BOOL init_joy(u16 joyCfg[]) {
  int i, nbJoysticks;

  /* Joystick configuration */
  memcpy(joypadCfg, joyCfg, sizeof(joypadCfg));

  if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1)
    {
      fprintf(stderr, "Error trying to initialize joystick support: %s\n",
              SDL_GetError());
      return FALSE;
    }

  nbJoysticks = SDL_NumJoysticks();
  printf("Nbr of joysticks: %d\n\n", nbJoysticks);

  for (i = 0; i < nbJoysticks; i++)
    {
      SDL_Joystick * joy = SDL_JoystickOpen(i);
      printf("Joystick %s\n", i, SDL_JoystickName(i));
      printf("Axes: %d\n", SDL_JoystickNumAxes(joy));
      printf("Buttons: %d\n", SDL_JoystickNumButtons(joy));
      printf("Trackballs: %d\n", SDL_JoystickNumBalls(joy));
      printf("Hats: %d\n\n", SDL_JoystickNumHats(joy));
    }
}

/* Unload joysticks */
void uninit_joy()
{
  /* FIXME: Should we Close all joysticks? 
  SDL_JoystickClose( ... );
  */
  SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

u16 inline lookup_joykey (u16 keyval) {
  int i;
  u16 Key = 0;
  for(i = 0; i < NB_KEYS; i++)
    if(keyval == joypadCfg[i]) break;
  if(i < NB_KEYS)	
    Key = KEYMASK_(i);
  printf("Lookup key %d from joypad...%x\n", keyval, Key);
  return Key;
}

#ifndef GTK_UI
/* Set mouse coordinates */
void mouse_set_coord(signed long x,signed long y)
{
  if(x<0) x = 0; else if(x>255) x = 255;
  if(y<0) y = 0; else if(y>192) y = 192;
  mouse_pos.x = x;
  mouse_pos.y = y;
}
#endif

/* Manage joystick events */
u16 process_ctrls_events(u16 keypad)
{
  u16 key;

  SDL_Event event;
#ifndef GTK_UI
  sdl_quit = FALSE;
#endif

  /* There's an event waiting to be processed? */
  while (SDL_PollEvent(&event))
    {
      switch (event.type)
	{
          /* Joystick axis motion */
        case SDL_JOYAXISMOTION:
          /* Horizontal */
          if (event.jaxis.axis == 0)
            if( event.jaxis.value == 0 )
              {
                key = lookup_joykey( 20 ) | lookup_joykey( 21 );
                RM_KEY( keypad, key );
              }
            else
              {
                if( event.jaxis.value > 0 ) key = lookup_joykey( 20 );
                else key = lookup_joykey( 21 );
                ADD_KEY( keypad, key );
              }
          /* Vertical */
          else if (event.jaxis.axis == 1)
            if( event.jaxis.value == 0 )
              {
                key = lookup_joykey( 22 ) | lookup_joykey( 23 );
                RM_KEY( keypad, key );
              }
            else
              {
                if( event.jaxis.value > 0 ) key = lookup_joykey( 23 );
                else key = lookup_joykey( 22 );
                ADD_KEY( keypad, key );
              }
          break;

          /* Joystick button pressed */
          /* FIXME: Add support for BOOST */
        case SDL_JOYBUTTONDOWN:
          key = lookup_joykey( event.jbutton.button );
          ADD_KEY( keypad, key );
          break;

          /* Joystick button released */
        case SDL_JOYBUTTONUP:
          key = lookup_joykey(event.jbutton.button);
          RM_KEY( keypad, key );
          break;

          /* GTK only needs joystick support. For others, we here provide the
             mouse, keyboard and quit events. */
#ifndef GTK_UI
        case SDL_KEYDOWN:
          switch (event.key.keysym.sym)
            {
            case SDLK_UP:       ADD_KEY( keypad, 0x40); break;
            case SDLK_DOWN:	ADD_KEY( keypad, 0x80); break;
            case SDLK_RIGHT:	ADD_KEY( keypad, 0x10); break;
            case SDLK_LEFT:	ADD_KEY( keypad, 0x20); break;
            case SDLK_SPACE:	ADD_KEY( keypad, 0x1); break;
            case 'b':	        ADD_KEY( keypad, 0x2); break;
            case SDLK_BACKSPACE:ADD_KEY( keypad, 0x4); break;
            case SDLK_RETURN:	ADD_KEY( keypad, 0x8); break;
            case '0':	        ADD_KEY( keypad, 0x200); break;
            case '.':	        ADD_KEY( keypad, 0x100); break;
            }
          break;

        case SDL_KEYUP:
          switch (event.key.keysym.sym)
            {
            case SDLK_UP:       RM_KEY( keypad, 0x40); break;
            case SDLK_DOWN:	RM_KEY( keypad, 0x80); break;
            case SDLK_RIGHT:	RM_KEY( keypad, 0x10); break;
            case SDLK_LEFT:	RM_KEY( keypad, 0x20); break;
            case SDLK_SPACE:	RM_KEY( keypad, 0x1); break;
            case 'b':	        RM_KEY( keypad, 0x2); break;
            case SDLK_BACKSPACE:RM_KEY( keypad, 0x4); break;
            case SDLK_RETURN:	RM_KEY( keypad, 0x8); break;
            case '0':	        RM_KEY( keypad, 0x200); break;
            case '.':	        RM_KEY( keypad, 0x100); break;
            }
          break;

        case SDL_MOUSEBUTTONDOWN: // Un bouton fut appuy�
          if(event.button.button==1)
            mouse_down = TRUE;
						
        case SDL_MOUSEMOTION: // La souris a �t� d�plac�e sur l?�cran
          if(!mouse_down) break;
          if(event.button.y>=192)
            mouse_set_coord(event.button.x, event.button.y - 192);
          break;

        case SDL_MOUSEBUTTONUP: // Le bouton de la souris a �t� relach�
          if(mouse_down) mouse_click = TRUE;
          mouse_down = FALSE;
          break;

        case SDL_QUIT:
          sdl_quit = TRUE;
          break;
#endif // GTK_UI

        default:
          break;
        }
    }
  return keypad;
}

