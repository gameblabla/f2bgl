/*
 * Fade To Black engine rewrite
 * Copyright (C) 2006-2012 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include <SDL/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "stub.h"

#ifdef EGL_CONTEXT
#include "eglport.h"
#endif

char home_directory[128];

const char *g_caption = "Fade2Black/OpenGL";

static const int kDefaultW = 320;
static const int kDefaultH = kDefaultW * 3 / 4;

static int gWindowW = kDefaultW;
static int gWindowH = kDefaultH;

static int gScale = 1;
static int gSaveSlot = 1;

static const int kTickDuration = 40;

static const int kJoystickIndex = 0;
static const int kJoystickCommitValue = 16384;

static const int kJoystickMapSize = 8;
static int gJoystickMap[kJoystickMapSize];

static int gKeySymMap[512];
static int gKeyScanMap[256];


static void setupKeyMap() {
	memset(gKeySymMap, 0, sizeof(gKeySymMap));
	gKeySymMap[SDLK_LEFT]   = kKeyCodeLeft;
	gKeySymMap[SDLK_RIGHT]  = kKeyCodeRight;
	gKeySymMap[SDLK_UP]     = kKeyCodeUp;
	gKeySymMap[SDLK_DOWN]   = kKeyCodeDown;
	/*
	gKeySymMap[SDLK_LALT]   = kKeyCodeAlt; // Pull gun
	gKeySymMap[SDLK_LSHIFT] = kKeyCodeShift; // Walk
	gKeySymMap[SDLK_LCTRL]  = kKeyCodeCtrl; // Shoot
	gKeySymMap[SDLK_SPACE]  = kKeyCodeSpace; // Use
	gKeySymMap[SDLK_RETURN] = kKeyCodeReturn; // Recharge
	gKeySymMap[SDLK_TAB]    = kKeyCodeTab;
	gKeySymMap[SDLK_ESCAPE] = kKeyCodeEscape;
	gKeySymMap[SDLK_i]      = kKeyCodeI;
	gKeySymMap[SDLK_j]      = kKeyCodeJ;
	*/
	
	/* B to shoot */ 
	gKeySymMap[SDLK_LALT]   = kKeyCodeCtrl;
	
	/* L to walk */
	gKeySymMap[SDLK_TAB] = kKeyCodeShift;
	
	/* Y to Open */
	gKeySymMap[SDLK_SPACE]  = kKeyCodeSpace;
	
	/* X to Pull gun */
	gKeySymMap[SDLK_LSHIFT]  = kKeyCodeAlt;
	
	/* R to Recharge */
	gKeySymMap[SDLK_BACKSPACE] = kKeyCodeReturn;
	
	/* Start for Inventory */
	gKeySymMap[SDLK_RETURN]      = kKeyCodeI;
	
	/* A to Jump */
	gKeySymMap[SDLK_LCTRL]      = kKeyCodeJ;
	
	
	memset(gKeyScanMap, 0, sizeof(gKeyScanMap));
	for (int i = 0; i < 5; ++i) {
		gKeyScanMap[10 + i] = kKeyCode1 + i;
	}
	// joystick buttons
	memset(gJoystickMap, 0, sizeof(gJoystickMap));
	gJoystickMap[0] = kKeyCodeAlt;
	gJoystickMap[1] = kKeyCodeShift;
	gJoystickMap[2] = kKeyCodeCtrl;
	gJoystickMap[3] = kKeyCodeReturn;
	gJoystickMap[4] = kKeyCodeSpace;
	gJoystickMap[5] = kKeyCodeI;
	gJoystickMap[6] = kKeyCodeU;
	gJoystickMap[7] = kKeyCodeJ;
}

static void lockAudio(int lock) {
	if (lock) {
		SDL_LockAudio();
	} else {
		SDL_UnlockAudio();
	}
}

static void setupAudio(GameStub *stub) {
	SDL_AudioSpec desired, obtained;
	memset(&desired, 0, sizeof(desired));
	desired.freq = 22050;
	desired.format = AUDIO_S16SYS;
	desired.channels = 2;
	desired.samples = 4096;
	StubMixProc mix = stub->getMixProc(desired.freq, desired.format, lockAudio);
	if (mix.proc) {
		desired.callback = mix.proc;
		desired.userdata = mix.data;
		if (SDL_OpenAudio(&desired, &obtained) == 0) {
			SDL_PauseAudio(0);
		}
	}
}

struct GetStub_impl {
	GameStub *getGameStub() {
		return GameStub_create();
	}
};

int main(int argc, char *argv[]) 
{
	snprintf(home_directory, sizeof(home_directory), "%s/.fadetoblack", getenv("HOME"));
	mkdir(home_directory, 0755);
	
	GetStub_impl gs;
	GameStub *stub = gs.getGameStub();
	if (!stub) {
		return -1;
	}
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);
#ifndef EGL_CONTEXT
	SDL_ShowCursor(SDL_DISABLE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_SetVideoMode(gWindowW, gWindowH, 0, SDL_OPENGL | SDL_RESIZABLE);
#else
	SDL_SetVideoMode(gWindowW, gWindowH, 0, SDL_HWSURFACE | SDL_TRIPLEBUF);
	EGL_Open(gWindowW, gWindowH);
#endif
	const int ret = stub->init(argc, argv);
	if (ret != 0) {
		return ret;
	}
	setupKeyMap();
	setupAudio(stub);
	SDL_Joystick *joystick = 0;
	if (SDL_NumJoysticks() > 0) {
		if (!joystick) {
			joystick = SDL_JoystickOpen(kJoystickIndex);
		}
	}
	
	stub->initGL(gWindowW, gWindowH);
	SDL_WM_SetCaption(g_caption, 0);
	bool quitGame = false;
	bool paused = false;
	while (1) 
	{
		int w = gWindowW;
		int h = gWindowH;
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) 
		{
			switch (ev.type) {
			case SDL_QUIT:
				quitGame = true;
				break;
			/*case SDL_ACTIVEEVENT:
				if (ev.active.state & SDL_APPINPUTFOCUS) {
					paused = !ev.active.gain;
					SDL_PauseAudio(paused);
				}
				break;
			case SDL_VIDEORESIZE:
				w = ev.resize.w;
				h = ev.resize.h;*/
			break;
			case SDL_KEYUP:
				if (gKeySymMap[ev.key.keysym.sym] != 0) {
					stub->queueKeyInput(gKeySymMap[ev.key.keysym.sym], 0);
				} else if (gKeyScanMap[ev.key.keysym.scancode] != 0) {
					stub->queueKeyInput(gKeyScanMap[ev.key.keysym.scancode], 0);
				}
				break;
			case SDL_KEYDOWN:
					switch (ev.key.keysym.sym) {
					case SDLK_ESCAPE:
						quitGame = true;
						break;
					default:
						break;
					}
				/*if (ev.key.keysym.mod & KMOD_MODE) 
				{
					switch (ev.key.keysym.sym) {
					default:
						break;
					}
					w = (int)(kDefaultW * gScale * .5);
					h = (int)(kDefaultH * gScale * .5);
					break;
				}*/
				if (gKeySymMap[ev.key.keysym.sym] != 0) {
					stub->queueKeyInput(gKeySymMap[ev.key.keysym.sym], 1);
				} else if (gKeyScanMap[ev.key.keysym.scancode] != 0) {
					stub->queueKeyInput(gKeyScanMap[ev.key.keysym.scancode], 1);
				}
			break;
			case SDL_JOYHATMOTION:
				if (joystick) 
				{
					stub->queueKeyInput(kKeyCodeUp,    (ev.jhat.value & SDL_HAT_UP)    != 0);
					stub->queueKeyInput(kKeyCodeDown,  (ev.jhat.value & SDL_HAT_DOWN)  != 0);
					stub->queueKeyInput(kKeyCodeLeft,  (ev.jhat.value & SDL_HAT_LEFT)  != 0);
					stub->queueKeyInput(kKeyCodeRight, (ev.jhat.value & SDL_HAT_RIGHT) != 0);
				}
				break;
			case SDL_JOYAXISMOTION:
				if (joystick) 
				{
					switch (ev.jaxis.axis) 
					{
					case 0:
						stub->queueKeyInput(kKeyCodeLeft,  (ev.jaxis.value < -kJoystickCommitValue));
						stub->queueKeyInput(kKeyCodeRight, (ev.jaxis.value >  kJoystickCommitValue));
						break;
					case 1:
						stub->queueKeyInput(kKeyCodeUp,   (ev.jaxis.value < -kJoystickCommitValue));
						stub->queueKeyInput(kKeyCodeDown, (ev.jaxis.value >  kJoystickCommitValue));
						break;
					}
				}
				break;
			/*case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
				if (joystick) 
				{
					if (ev.jbutton.button >= 0 && ev.jbutton.button < kJoystickMapSize) 
					{
						if (gJoystickMap[ev.jbutton.button] != 0) 
						{
							stub->queueKeyInput(gJoystickMap[ev.jbutton.button], ev.jbutton.state == SDL_PRESSED);
						}
					}
				}
				break;*/
			default:
				break;
			}
		}
		
		if (quitGame) 
		{
			break;
		}
		if (w != gWindowW || h != gWindowH) 
		{
			gWindowW = w;
			gWindowH = h;
#ifndef EGL_CONTEXT
			SDL_SetVideoMode(gWindowW, gWindowH, 0, SDL_OPENGL | SDL_RESIZABLE);
#else
			SDL_SetVideoMode(gWindowW, gWindowH, 0, SDL_HWSURFACE | SDL_DOUBLEBUF);
			EGL_Open(gWindowW, gWindowH);
#endif
		}
		/*if (!paused) 
		{*/
			const unsigned int ticks = SDL_GetTicks();
			stub->doTick(ticks);
			stub->drawGL();
#ifndef EGL_CONTEXT
			SDL_GL_SwapBuffers();
#else
			EGL_SwapBuffers();
#endif
		/*}
		msleep(kTickDuration);*/
	}
	
#ifdef EGL_CONTEXT
	EGL_Close();
#endif
	SDL_PauseAudio(1);
	stub->quit();
	if (joystick) 
	{
		SDL_JoystickClose(joystick);
	}
	SDL_Quit();
	return 0;
}



