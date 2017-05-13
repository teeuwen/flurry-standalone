/* -*- Mode: C; tab-width: 4 c-basic-offset: 4 indent-tabs-mode: t -*- */
/*
 * vim: ts=8 sw=4 noet
 */

/*

Copyright (c) 2002, Calum Robinson
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the author nor the names of its contributors may be used
  to endorse or promote products derived from this software without specific
  prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <sys/time.h>

#include <X11/X.h>
#include <X11/Xlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <flurry.h>

#define DEF_PRESET     "random"
#define DEF_BRIGHTNESS "8"

# define DEFAULTS		"*delay:      10000 \n" \
						"*showFPS:    False \n"

# define refresh_flurry 0
# define flurry_handle_event 0

static char *preset_str;

global_info_t *flurry_info = NULL;

static double gTimeCounter = 0.0;

static
double currentTime(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

void OTSetup (void) {
    if (gTimeCounter == 0.0) {
        gTimeCounter = currentTime();
    }
}

double TimeInSecondsSinceStart (void) {
    return currentTime() - gTimeCounter;
}

static
void delete_flurry_info(flurry_info_t *flurry)
{
    int i;

    free(flurry->s);
    free(flurry->star);
    for (i=0;i<MAX_SPARKS;i++)
    {
	free(flurry->spark[i]);
    }
    /* free(flurry); */
}

static
flurry_info_t *new_flurry_info(global_info_t *global, int streams, ColorModes colour, float thickness, float speed, double bf)
{
    int i,k;
    flurry_info_t *flurry = (flurry_info_t *)malloc(sizeof(flurry_info_t));

    if (!flurry) return NULL;

    flurry->flurryRandomSeed = RandFlt(0.0, 300.0);

	flurry->fOldTime = 0;
	flurry->fTime = TimeInSecondsSinceStart() + flurry->flurryRandomSeed;
 	flurry->fDeltaTime = flurry->fTime - flurry->fOldTime;

    flurry->numStreams = streams;
    flurry->streamExpansion = thickness;
    flurry->currentColorMode = colour;
    flurry->briteFactor = bf;

    flurry->s = malloc(sizeof(SmokeV));
    InitSmoke(flurry->s);

    flurry->star = malloc(sizeof(Star));
    InitStar(flurry->star);
    flurry->star->rotSpeed = speed;

    for (i = 0;i < MAX_SPARKS; i++)
    {
	flurry->spark[i] = malloc(sizeof(Spark));
	InitSpark(flurry->spark[i]);
	flurry->spark[i]->mystery = 1800 * (i + 1) / 13; /* 100 * (i + 1) / (flurry->numStreams + 1); */
	UpdateSpark(global, flurry, flurry->spark[i]);
    }

    for (i=0;i<NUMSMOKEPARTICLES/4;i++) {
	for(k=0;k<4;k++) {
	    flurry->s->p[i].dead.i[k] = 1;
	}
    }

    flurry->next = NULL;

    return flurry;
}

static GLXContext *init_GL(Display *dpy, Window win)
{
  GLXContext glx_context = 0;
  XVisualInfo vi_in, *vi_out;
  int out_count;
  XWindowAttributes wa;

  /* TODO CHECKS */
  XGetWindowAttributes(dpy, win, &wa);

  Visual *visual = wa.visual;

# ifdef HAVE_JWZGLES
  jwzgles_reset();
# endif

  vi_in.screen = 0;
  vi_in.visualid = XVisualIDFromVisual (visual);
  vi_out = XGetVisualInfo (dpy, VisualScreenMask|VisualIDMask,
			   &vi_in, &out_count);
  if (! vi_out) abort ();

  {
    XSync (dpy, False);
    /* orig_ehandler = XSetErrorHandler (BadValue_ehandler); */
    glx_context = glXCreateContext (dpy, vi_out, 0, GL_TRUE);
    XSync (dpy, False);
    /* XSetErrorHandler (orig_ehandler); */
  }

  XFree((char *) vi_out);

  if (!glx_context)
      exit(1);

  glXMakeCurrent (dpy, win, glx_context);

  {
    GLboolean rgba_mode = 0;
    glGetBooleanv(GL_RGBA_MODE, &rgba_mode);
  }


  /* jwz: the doc for glDrawBuffer says "The initial value is GL_FRONT
     for single-buffered contexts, and GL_BACK for double-buffered
     contexts."  However, I find that this is not always the case,
     at least with Mesa 3.4.2 -- sometimes the default seems to be
     GL_FRONT even when glGet(GL_DOUBLEBUFFER) is true.  So, let's
     make sure.

     Oh, hmm -- maybe this only happens when we are re-using the
     xscreensaver window, and the previous GL hack happened to die with
     the other buffer selected?  I'm not sure.  Anyway, this fixes it.
   */
  {
    GLboolean d = False;
    glGetBooleanv (GL_DOUBLEBUFFER, &d);
    if (d)
      glDrawBuffer (GL_BACK);
    else
      glDrawBuffer (GL_FRONT);
  }

  /* Sometimes glDrawBuffer() throws "invalid op". Dunno why. Ignore. */
  /* clear_gl_error (); */

  /* Process the -background argument. */
  {
    /* char *s = get_string_resource(dpy, "background", "Background");
    XColor c = { 0, };
    if (! XParseColor (dpy, mi->xgwa.colormap, s, &c))
      fprintf (stderr, "%s: can't parse color %s; using black.\n", 
               progname, s);
    glClearColor (c.red   / 65535.0,
                  c.green / 65535.0,
                  c.blue  / 65535.0,
                  1.0); */
  }

  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /* GLXContext is already a pointer type.
     Why this function returns a pointer to a pointer, I have no idea...
   */
  {
    GLXContext *ptr = (GLXContext *) malloc(sizeof(GLXContext));
    *ptr = glx_context;
    return ptr;
  }
}

static
void GLSetupRC(global_info_t *global)
{
    /* setup the defaults for OpenGL */
    glDisable(GL_DEPTH_TEST);
    glAlphaFunc(GL_GREATER,0.0f);
    glEnable(GL_ALPHA_TEST);
    glShadeModel(GL_FLAT);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);

    glViewport(0,0,(int) global->sys_glWidth,(int) global->sys_glHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,global->sys_glWidth,0,global->sys_glHeight,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClear(GL_COLOR_BUFFER_BIT);

    glEnableClientState(GL_COLOR_ARRAY);	
    glEnableClientState(GL_VERTEX_ARRAY);	
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}

static
void GLRenderScene(global_info_t *global, flurry_info_t *flurry, double b)
{
    int i;

    flurry->dframe++;

    flurry->fOldTime = flurry->fTime;
    flurry->fTime = TimeInSecondsSinceStart() + flurry->flurryRandomSeed;
    flurry->fDeltaTime = flurry->fTime - flurry->fOldTime;

    flurry->drag = (float) pow(0.9965,flurry->fDeltaTime*85.0);

    UpdateStar(global, flurry, flurry->star);

#ifdef DRAW_SPARKS
    glShadeModel(GL_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
#endif

    for (i=0;i<flurry->numStreams;i++) {
	flurry->spark[i]->color[0]=1.0;
	flurry->spark[i]->color[1]=1.0;
	flurry->spark[i]->color[2]=1.0;
	flurry->spark[i]->color[2]=1.0;
	UpdateSpark(global, flurry, flurry->spark[i]);
#ifdef DRAW_SPARKS
	DrawSpark(global, flurry, flurry->spark[i]);
#endif
    }

    switch(global->optMode) {
	case OPT_MODE_SCALAR_BASE:
	    UpdateSmoke_ScalarBase(global, flurry, flurry->s);
	    break;

	default:
	    break;
    }

    /* glDisable(GL_BLEND); */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    glEnable(GL_TEXTURE_2D);

    switch(global->optMode) {
	case OPT_MODE_SCALAR_BASE:
	    DrawSmoke_Scalar(global, flurry, flurry->s, b);
	    break;
	default:
	    break;
    }

    glDisable(GL_TEXTURE_2D);
}

static
void GLResize(global_info_t *global, float w, float h)
{
    global->sys_glWidth = w;
    global->sys_glHeight = h;
}

/* new window size or exposure */
static void reshape_flurry(Display *dpy, int width, int height)
{
    global_info_t *global = flurry_info;

    glXMakeCurrent(dpy, global->window, *(global->glx_context));

    glViewport(0.0, 0.0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, 0, height,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();
    GLResize(global, (float)width, (float)height);
}

static void init_flurry(Display *dpy, Window win)
{
    int i;
    global_info_t *global;
    enum {
	PRESET_INSANE = -1,
        PRESET_WATER = 0,
        PRESET_FIRE,
        PRESET_PSYCHEDELIC,
        PRESET_RGB,
        PRESET_BINARY,
        PRESET_CLASSIC,
        PRESET_MAX
    } preset_num;

    if (flurry_info == NULL) {
	OTSetup();
	if ((flurry_info = (global_info_t *) calloc(1,
			sizeof (global_info_t))) == NULL)
	    return;
    }

    global = &flurry_info[0];

    global->window = win;

    global->flurry = NULL;

    if (!preset_str || !*preset_str) preset_str = DEF_PRESET;
    if (!strcmp(preset_str, "random")) {
        preset_num = random() % PRESET_MAX;
    } else if (!strcmp(preset_str, "water")) {
        preset_num = PRESET_WATER;
    } else if (!strcmp(preset_str, "fire")) {
        preset_num = PRESET_FIRE;
    } else if (!strcmp(preset_str, "psychedelic")) {
        preset_num = PRESET_PSYCHEDELIC;
    } else if (!strcmp(preset_str, "rgb")) {
        preset_num = PRESET_RGB;
    } else if (!strcmp(preset_str, "binary")) {
        preset_num = PRESET_BINARY;
    } else if (!strcmp(preset_str, "classic")) {
        preset_num = PRESET_CLASSIC;
    } else if (!strcmp(preset_str, "insane")) {
        preset_num = PRESET_INSANE;
    } else {
        exit(1);
    }

    switch (preset_num) {
    case PRESET_WATER: {
	for (i = 0; i < 9; i++) {
	    flurry_info_t *flurry;

	    flurry = new_flurry_info(global, 1, blueColorMode, 100.0, 2.0, 2.0);
	    flurry->next = global->flurry;
	    global->flurry = flurry;
	}
        break;
    }
    case PRESET_FIRE: {
	flurry_info_t *flurry;

	flurry = new_flurry_info(global, 12, slowCyclicColorMode, 10000.0, 0.2, 1.0);
	flurry->next = global->flurry;
	global->flurry = flurry;
        break;
    }
    case PRESET_PSYCHEDELIC: {
	flurry_info_t *flurry;

	flurry = new_flurry_info(global, 10, rainbowColorMode, 200.0, 2.0, 1.0);
	flurry->next = global->flurry;
	global->flurry = flurry;	
        break;
    }
    case PRESET_RGB: {
	flurry_info_t *flurry;

	flurry = new_flurry_info(global, 3, redColorMode, 100.0, 0.8, 1.0);
	flurry->next = global->flurry;
	global->flurry = flurry;	

	flurry = new_flurry_info(global, 3, greenColorMode, 100.0, 0.8, 1.0);
	flurry->next = global->flurry;
	global->flurry = flurry;	

	flurry = new_flurry_info(global, 3, blueColorMode, 100.0, 0.8, 1.0);
	flurry->next = global->flurry;
	global->flurry = flurry;	
        break;
    }
    case PRESET_BINARY: {
	flurry_info_t *flurry;

	flurry = new_flurry_info(global, 16, tiedyeColorMode, 1000.0, 0.5, 1.0);
	flurry->next = global->flurry;
	global->flurry = flurry;

	flurry = new_flurry_info(global, 16, tiedyeColorMode, 1000.0, 1.5, 1.0);
	flurry->next = global->flurry;
	global->flurry = flurry;
        break;
    }
    case PRESET_CLASSIC: {
	flurry_info_t *flurry;

	flurry = new_flurry_info(global, 5, tiedyeColorMode, 10000.0, 1.0, 1.0);
	flurry->next = global->flurry;
	global->flurry = flurry;
        break;
    }
    case PRESET_INSANE: {
	flurry_info_t *flurry;

	flurry = new_flurry_info(global, 64, tiedyeColorMode, 1000.0, 0.5, 0.5);
	flurry->next = global->flurry;
	global->flurry = flurry;

        break;
    }
    default: {
        exit(1);
    }
    } 

    if ((global->glx_context = init_GL(dpy, win)) != NULL) {
	    /* XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX */
	reshape_flurry(dpy, 1366, 768);
	GLSetupRC(global);
    }
}

static void draw_flurry(Display *dpy, Window win)
{
    static int first = 1;
    static double oldFrameTime = -1;
    double newFrameTime;
    double deltaFrameTime = 0;
    double brite;
    GLfloat alpha;

    global_info_t *global = flurry_info;
    flurry_info_t *flurry;

    newFrameTime = currentTime();
    if (oldFrameTime == -1) {
	/* special case the first frame -- clear to black */
	alpha = 1.0;
    } else {
	/* 
	 * this clamps the speed at below 60fps and, here
	 * at least, produces a reasonably accurate 50fps.
	 * (probably part CPU speed and part scheduler).
	 *
	 * Flurry is designed to run at this speed; much higher
	 * than that and the blending causes the display to
	 * saturate, which looks really ugly.
	 */
	if (newFrameTime - oldFrameTime < 1/60.0) {
	    usleep(MAX_(1,(int)(20000 * (newFrameTime - oldFrameTime))));
	    return;

	}
	deltaFrameTime = newFrameTime - oldFrameTime;
	alpha = 5.0 * deltaFrameTime;
    }
    oldFrameTime = newFrameTime;

    if (alpha > 0.2) alpha = 0.2;

    if (!global->glx_context)
	return;

    if (first) {
	MakeTexture();
	first = 0;
    }
    glDrawBuffer(GL_BACK);
    glXMakeCurrent(dpy, win, *(global->glx_context));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.0, 0.0, 0.0, alpha);
    glRectd(0, 0, global->sys_glWidth, global->sys_glHeight);

    brite = pow(deltaFrameTime,0.75) * 10;
    for (flurry = global->flurry; flurry; flurry=flurry->next) {
	GLRenderScene(global, flurry, brite * flurry->briteFactor);
    }

    glFinish();
    glXSwapBuffers(dpy, win);
}

#if 0
static void release_flurry(ModeInfo * mi)
{
    if (flurry_info != NULL) {
	int screen;

	global_info_t *global = &flurry_info[screen];
	flurry_info_t *flurry;

	if (global->glx_context) {
	    glXMakeCurrent(MI_DISPLAY(mi), global->window, *(global->glx_context));
	}

	for (flurry = global->flurry; flurry; flurry=flurry->next) {
	    delete_flurry_info(flurry);
	}
	(void) free((void *) flurry_info);
	flurry_info = NULL;
    }
    FreeAllGL(mi);
}
#endif

int main(int argc, char **argv)
{
	Display *dpy;
	Window win;

	if (!(dpy = XOpenDisplay(NULL)))
		return 1;

	if (!(win = XCreateSimpleWindow(dpy, RootWindow(dpy, 0), 0, 0, 10, 10,
					0, BlackPixel(dpy, 0),
					BlackPixel(dpy, 0))))
		return 1;

	Atom wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
	Atom fullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);

	XEvent xev;
    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.xclient.window = win;
    xev.xclient.message_type = wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 1;
    xev.xclient.data.l[1] = fullscreen;
    xev.xclient.data.l[2] = 0;

    XMapWindow(dpy, win);

    XSendEvent (dpy, DefaultRootWindow(dpy), False,
                    SubstructureRedirectMask | SubstructureNotifyMask, &xev);

    XFlush(dpy);

    init_flurry(dpy, win);

    for(;;)
	draw_flurry(dpy, win);
}
