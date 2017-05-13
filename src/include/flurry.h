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

#ifndef _FLURRY_H
#define _FLURRY_H

/* -*- Mode: C; tab-width: 4 c-basic-offset: 4 indent-tabs-mode: t -*- */
/* flurry */
#include <GL/glx.h>

#include <stdlib.h>
#include <math.h>

typedef struct _global_info_t global_info_t;
typedef struct _flurry_info_t flurry_info_t;

static double _frand_tmp_;
# define frand(f)							\
  (_frand_tmp_ = ((((double) random()) * ((double) (f))) /		\
		  ((double) ((unsigned int)~0))),			\
   _frand_tmp_ < 0 ? (-_frand_tmp_) : _frand_tmp_)

#define sqr(X)     ((X) * (X))
#define PI         3.14159265358979323846f
#define DEG2RAD(X) (PI*(X)/180.0)
#define RAD2DEG(X) ((X)*180.0/PI)
#define rnd()      (frand(1.0))

/* fabs: Absolute function. */
/* #undef abs */
/* #define abs(a)     ( (a) > 0 ? (a) : -(a) ) */

/* Force sign clamping to (-1;0;1) */
#define sgn(a)      ((a)<0?-1:((a)?1:0))

/* used to compute the min and max of two expresions */
#define MIN_(a, b)  (((a) < (b)) ? (a) : (b)) 
#define MAX_(a, b)  (((a) > (b)) ? (a) : (b)) 

typedef union {
    float		f[4];
} floatToVector;

typedef union {
    unsigned int	i[4];
} intToVector;

typedef struct SmokeParticleV  
{
	floatToVector color[4];
	floatToVector position[3];
	floatToVector oldposition[3];
	floatToVector delta[3];
	intToVector dead;
	floatToVector time;
	intToVector animFrame;
} SmokeParticleV;

#define NUMSMOKEPARTICLES 3600

typedef struct SmokeV  
{
	SmokeParticleV p[NUMSMOKEPARTICLES/4];
	int nextParticle;
        int nextSubParticle;
	float lastParticleTime;
	int firstTime;
	long frame;
	float old[3];
        floatToVector seraphimVertices[NUMSMOKEPARTICLES*2+1];
        floatToVector seraphimColors[NUMSMOKEPARTICLES*4+1];
	float seraphimTextures[NUMSMOKEPARTICLES*2*4];
} SmokeV;

void InitSmoke(SmokeV *s);

void UpdateSmoke_ScalarBase(global_info_t *global, flurry_info_t *flurry, SmokeV *s);

void DrawSmoke_Scalar(global_info_t *global, flurry_info_t *flurry, SmokeV *s, float);
void DrawSmoke_Vector(global_info_t *global, flurry_info_t *flurry, SmokeV *s, float);

typedef struct Star  
{
	float position[3];
	float mystery;
	float rotSpeed;
	int ate;
} Star;

void UpdateStar(global_info_t *global, flurry_info_t *flurry, Star *s);
void InitStar(Star *s);

typedef struct Spark  
{
    float position[3];
    int mystery;
    float delta[3];
    float color[4];    
} Spark;

void UpdateSparkColour(global_info_t *info, flurry_info_t *flurry, Spark *s);
void InitSpark(Spark *s);
void UpdateSpark(global_info_t *info, flurry_info_t *flurry, Spark *s);
void DrawSpark(global_info_t *info, flurry_info_t *flurry, Spark *s);

/* UInt8  sys_glBPP=32; */
/* int SSMODE = FALSE; */
/* int currentVideoMode = 0; */
/* int cohesiveness = 7; */
/* int fieldStrength; */
/* int colorCoherence = 7; */
/* int fieldIncoherence = 0; */
/* int ifieldSpeed = 120; */

#define RandFlt(min, max) ((min) + frand((max) - (min)))

#define RandBell(scale) ((scale) * (-(frand(.5) + frand(.5) + frand(.5))))

extern GLuint theTexture;

void MakeTexture(void);

#define OPT_MODE_SCALAR_BASE		0x0

typedef enum _ColorModes
{
	redColorMode = 0,
	magentaColorMode,
	blueColorMode,
	cyanColorMode,
	greenColorMode,
	yellowColorMode,
	slowCyclicColorMode,
	cyclicColorMode,
	tiedyeColorMode,
	rainbowColorMode,
	whiteColorMode,
	multiColorMode,
	darkColorMode
} ColorModes;

#define gravity 1500000.0f

#define incohesion 0.07f
#define colorIncoherence 0.15f
#define streamSpeed 450.0
#define fieldCoherence 0
#define fieldSpeed 12.0f
#define numParticles 250
#define starSpeed 50
#define seraphDistance 2000.0f
#define streamSize 25000.0f
#define fieldRange 1000.0f
#define streamBias 7.0f

#define MAX_SPARKS 64

struct _flurry_info_t {
	flurry_info_t *next;
	ColorModes currentColorMode;
	SmokeV *s;
	Star *star;
	Spark *spark[MAX_SPARKS];
	float streamExpansion;
	int numStreams;
	double flurryRandomSeed;
	double fTime;
	double fOldTime;
	double fDeltaTime;
	double briteFactor;
	float drag;
	int dframe;
};

struct _global_info_t {
  /* system values */
	GLXContext *glx_context;
	Window window;
        int optMode;

	float sys_glWidth;
	float sys_glHeight;

	flurry_info_t *flurry;
};

#define kNumSpectrumEntries 512

void OTSetup(void);
double TimeInSecondsSinceStart(void);

#endif /* Include/Define */
