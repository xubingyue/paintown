#ifndef _paintown_init_h
#define _paintown_init_h

// #include <allegro.h>
#include <pthread.h>


/* global vars */

#define GFX_X 640
#define GFX_Y 480

namespace Global{
	extern volatile int speed_counter;
	extern volatile int second_counter;

        extern const double LOGIC_MULTIPLIER;
        extern const int TICS_PER_SECOND;

	extern pthread_mutex_t loading_screen_mutex;
	extern bool done_loading;

	// extern DATAFILE * all_fonts;
	
	extern const int WINDOWED;
	extern const int FULLSCREEN;

	bool init( int gfx );
}

/*
#define GFX_X 320
#define GFX_Y 240
*/

#endif
