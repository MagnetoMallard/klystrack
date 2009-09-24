/*
Copyright (c) 2009 Tero Lindeman (kometbomb)

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include "SDL.h"
#include "SDL_mixer.h"
#include "snd/music.h"
#include "toolutil.h"
#include "copypaste.h"
#include "toolutil.h"
#include "diskop.h"
#include "event.h"
#include "view.h"
#include "action.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#include "mused.h"
Mused mused;

/*---*/

int stat_song_position;
int stat_pattern_position[MUS_CHANNELS];
MusPattern *stat_pattern[MUS_CHANNELS];
int stat_pattern_number[MUS_CHANNELS];

static const View tab[] = 
{ 
	{{0,0, SCREEN_WIDTH, SCREEN_HEIGHT}, instrument_view},
	{{0,0, SCREEN_WIDTH, SCREEN_HEIGHT}, pattern_view},
	{{0,0, SCREEN_WIDTH, SCREEN_HEIGHT}, sequence_view} 
};

// mingw kludge for console output
#ifdef DEBUG
#undef main
#endif

int main(int argc, char **argv)
{
	SDL_Init(SDL_INIT_AUDIO|SDL_INIT_NOPARACHUTE);
	atexit(SDL_Quit);

	SDL_Surface *screen=SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_EnableUNICODE(1);
	
	int done = 0;
	mused.console = console_create(screen);
	
	MusInstrument instrument[NUM_INSTRUMENTS];
	MusPattern pattern[NUM_PATTERNS];
	MusSeqPattern sequence[MUS_CHANNELS][NUM_SEQUENCES];
	
	init(instrument, pattern, sequence);
	
	Mix_OpenAudio(44100, AUDIO_S16SYS, 1, 2048);
	Mix_AllocateChannels(1);
	
	cyd_init(&mused.cyd, 44100, MUS_CHANNELS);
	mus_init_engine(&mused.mus, &mused.cyd);
	
	cyd_register(&mused.cyd);
	cyd_set_callback(&mused.cyd, mus_advance_tick, &mused.mus, mused.song.song_rate);
	
	while (1)
	{
		SDL_Event e = { 0 };
		int got_event = 0;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
				case SDL_QUIT:
				done = 1;
				break;
				
				case SDL_KEYDOWN:
				{
					// key events should go only to the edited text field
					
					if (mused.mode != EDITBUFFER) 
					{
						switch (e.key.keysym.sym)
						{
							case SDLK_ESCAPE:
							done = 1;
							break;
							
							case SDLK_F9:
							
							if ((e.key.keysym.mod & (KMOD_CTRL|KMOD_SHIFT)) == (KMOD_CTRL|KMOD_SHIFT))
							{
								change_time_signature((void*)0, 0, 0);
							}
							else if (e.key.keysym.mod & KMOD_SHIFT)
							{
								change_song_rate((void*)-1, 0, 0);
							}
							else
							{
								change_octave((void*)-1, 0, 0);
							}
							
							break;
							
							case SDLK_F10:
							
							if ((e.key.keysym.mod & (KMOD_CTRL|KMOD_SHIFT)) == (KMOD_CTRL|KMOD_SHIFT))
							{
								change_time_signature((void*)1, 0, 0);
							}
							else if (e.key.keysym.mod & KMOD_SHIFT)
							{
								change_song_rate((void*)+1, 0, 0);
							}
							else
							{
								change_octave((void*)+1, 0, 0);
							}
							
							break;
							
							case SDLK_KP_PLUS:
							
							if (e.key.keysym.mod & KMOD_CTRL)
							{
								change_song_speed(0, (void*)+1, 0);
							}
							else if (e.key.keysym.mod & KMOD_ALT)
							{
								change_song_speed((void*)1, (void*)+1, 0);
							}
							else
							{
								select_instrument((void*)(mused.current_instrument + 1), 0, 0);
							}
							
							break;
							
							case SDLK_KP_MINUS:
							
							if (e.key.keysym.mod & KMOD_CTRL)
							{
								change_song_speed(0, (void*)-1, 0);
							}
							else if (e.key.keysym.mod & KMOD_ALT)
							{
								change_song_speed((void*)1, (void*)-1, 0);
							}
							else
							{
								select_instrument((void*)(mused.current_instrument - 1), 0, 0);
							}
							
							break;
							
							case SDLK_F2:
							change_mode(EDITPATTERN);
							break;
							
							case SDLK_F3:
							change_mode(EDITINSTRUMENT);
							break;
							
							case SDLK_F4:
							change_mode(EDITSEQUENCE);
							break;
							
							case SDLK_F5:
							play(0, 0, 0);
							break;
							
							case SDLK_F6:
							play((void*)mused.current_sequencepos, 0, 0);
							break;
							
							case SDLK_F8:
							stop(0, 0, 0);
							break;
							
							case SDLK_n:
							if (e.key.keysym.mod & KMOD_CTRL)
							{
								new_song_action(0, 0, 0);
								e.key.keysym.sym = 0;
							}
							break;
							
							case SDLK_s:
							if (e.key.keysym.mod & KMOD_CTRL)
							{
								save_song_action(0, 0, 0);
								e.key.keysym.sym = 0;
							}
							break;
							
							case SDLK_o:
							if (e.key.keysym.mod & KMOD_CTRL)
							{	
								open_song_action(0, 0, 0);
								e.key.keysym.sym = 0;
							}
							break;
							
							case SDLK_c:
							if (e.key.keysym.mod & KMOD_CTRL)
							{	
								generic_action(copy, 0, 0);
								e.key.keysym.sym = 0;
							}
							break;
							
							case SDLK_v:
							if (e.key.keysym.mod & KMOD_CTRL)
							{	
								generic_action(paste, 0, 0);
								e.key.keysym.sym = 0;
							}
							break;
							
							case SDLK_x:
							if (e.key.keysym.mod & KMOD_CTRL)
							{	
								generic_action(cut, 0, 0);
								e.key.keysym.sym = 0;
							}
							break;
							
							case SDLK_DELETE:
							if (e.key.keysym.mod & KMOD_SHIFT)
							{	
								generic_action(delete, 0, 0);
								e.key.keysym.sym = 0;
							}
							break;
							
							case SDLK_INSERT:
							if (e.key.keysym.mod & KMOD_SHIFT)
							{	
								generic_action(paste, 0, 0);
								e.key.keysym.sym = 0;
							}
							else if (e.key.keysym.mod & KMOD_CTRL)
							{	
								generic_action(copy, 0, 0);
								e.key.keysym.sym = 0;
							}
							break;
							
							default:
							break;
						}
					}
					
					if (e.key.keysym.sym != 0)
					{
						cyd_lock(&mused.cyd, 1);
						
						switch (mused.mode)
						{
							case EDITBUFFER:
							edit_text(&e);
							break;
							
							case EDITPROG:
							edit_program_event(&e);
							break;
							
							case EDITINSTRUMENT:
							edit_instrument_event(&e);
							break;
							
							case EDITPATTERN:
							pattern_event(&e);
							break;
							
							case EDITSEQUENCE:
							sequence_event(&e);
							break;
						}
						cyd_lock(&mused.cyd, 0);
					}
				}
				break;
			}
			
			++got_event;
			
			// ensure the last event is a mouse click so it gets passed to the draw/event code
			
			if (e.type == SDL_MOUSEBUTTONDOWN) break; 
		}
		
		mus_poll_status(&mused.mus, &mused.stat_song_position, mused.stat_pattern_position, mused.stat_pattern);
		
		for (int i = 0 ; i < MUS_CHANNELS ; ++i)
		{
			stat_pattern_number[i] = (stat_pattern[i] - &mused.song.pattern[0])/sizeof(mused.song.pattern[0]);
		}
		
		int m = (mused.mode == EDITBUFFER || mused.mode == EDITPROG) ? mused.prev_mode : mused.mode;
		
		/*{
			SDL_Rect dest = {0,0, SCREEN_WIDTH, SCREEN_HEIGHT-12};
			
			switch (m)
			{
				
				case EDITPATTERN:
				pattern_view(&dest, &e);
				break;
				
				case EDITSEQUENCE:
				sequence_view(&dest, &e);
				break;
			
				case EDITPROG:
				case EDITINSTRUMENT:
				instrument_view(&dest, &e);
				break;
			}
			
			dest.x = SCREEN_WIDTH - 200;
			
			info_view(&dest, &e);
		}
		
		{
			SDL_Rect dest = {0,SCREEN_HEIGHT-12, SCREEN_WIDTH, 12};
			info_line(&dest, &e);
		}*/
		
		draw_view(&tab[m], &e);
		
		SDL_Flip(screen);
		SDL_Delay(got_event ? 0 : 10);
		
		if (done) 
		{
			int r = confirm_ync("Save song?");
			
			if (r == 0) done = 0;
			if (r == -1) break;
			if (r == 1) { change_mode(EDITSEQUENCE); if (!save_data()) done = 0; else break; }
		}
	}
	
	Mix_CloseAudio();
	
	console_destroy(mused.console);
	cyd_unregister(&mused.cyd);
	cyd_deinit(&mused.cyd);
	
	return 0;
}
