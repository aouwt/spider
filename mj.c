#include <SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

#define SIN sinl
#define COS cosl
#define TAN tanl
#define ASIN asinl
#define ACOS acosl
#define ATAN atanl
#define POW powl
#define SQRT sqrtl
#define ABS fabsl


#define FRAME_TIME (1000 / 60)


#define THROWSDLERR	fprintf (stderr, "SDL error at %u: %s\n", (unsigned int) __LINE__, SDL_GetError ())

#define SDLERRNZ(func)	{	\
	if ((func) != 0) {	\
		THROWSDLERR;	\
		exit (1);	\
	}	\
}

#define SDLERRNULL(func)	{	\
	if ((func) == NULL) {	\
		THROWSDLERR;	\
		exit (1);	\
	}	\
}


typedef uint_fast32_t Word;

typedef float Float;
typedef Word Time;

typedef Float TAngle;
typedef Float TSide;

typedef struct _Point {
	Float x, y;
} Point;

typedef struct _Triangle {
	TSide a, b, c;
	TAngle A, B, C;
} Triangle;

typedef struct _Coord {
	int x, y;
} Coord;

typedef struct _TriangleCoord {
	Coord A, B, C;
} TriangleCoord;

struct _AttachTo {
	Coord l [9];
} *AttachTo;

SDL_Renderer *Renderer = NULL;
SDL_Window *Window = NULL;
SDL_Surface *BackgroundSurface = NULL;
SDL_Texture *BackgroundTexture = NULL;
SDL_Event Event;

int MouseX, MouseY;
Point Leg [9] = { 0 };


void GenAttachTable (void) {
	
	SDL_PixelFormat *fmt;
	SDLERRNULL	(fmt = SDL_AllocFormat (SDL_BYTESPERPIXEL (8)));
	
	SDL_Surface *img;
	SDLERRNULL	(img = SDL_ConvertSurface (BackgroundSurface, fmt, 0));
	
	SDL_Rect v;
	SDL_RenderGetViewport (Renderer, &v);
	
	AttachTo = malloc (sizeof (AttachTo [0]) * v->w * v->h);
	
	SDLERRNZ	(SDL_LockSurface (img));
	
	for (int px = 0; px != v->w; px ++) {
		for (int py = 0; py != v->h; py ++) {
			for (unsigned short leg = 0; leg != 8; leg ++) {
				int x, y;
				if (leg > 4)
					x = px - 64;
				else
					x = px + 64;
				y = py + ((py % 5) * 16);
				
				// do things or whatever
			}
		}
	}
	
	SDL_UnlockSurface (img);
	SDL_FreeSurface (img);
	SDL_FreeFormat (fmt);
}

void SolveTriangle_ABC (Triangle *t) {
	t->A = ACOS ((POW (t->b,2) + POW (t->c,2) - POW (t->a,2)) / (2 * t->b * t->c));
	t->B = ACOS ((POW (t->a,2) + POW (t->c,2) - POW (t->b,2)) / (2 * t->a * t->c));
	t->C = ACOS ((POW (t->a,2) + POW (t->b,2) - POW (t->c,2)) / (2 * t->a * t->b));
}

void GetTriangleCoord (const Point *C, const Triangle *t, TriangleCoord *c, signed short flip) {
	// A = (0, 0)
	// B = ((Cx cos(alpha) - Cy sin(alpha)) * c/b, (Cy cos(alpha) + Cx sin(alpha)) * c/b)
	// C = (Cx, Cy)

	c->A.x = c->A.y = 0;
	c->C = *C;
	
	const TAngle _cos = COS (t->A);
	const TAngle _sin = SIN (t->A) * (TAngle) flip;
	const Float mult = t->c / t->b;
	c->B.x = (c->C.x * _cos - c->C.y * _sin) * mult;
	c->B.y = (c->C.y * _cos + c->C.x * _sin) * mult;
}

void DrawTriangle (const TriangleCoord *c, const Point *a) {
	SDL_Point p [3];
	p [0].x = c->A.x + a->x; p [0].y = c->A.y + a->y;
	p [1].x = c->B.x + a->x; p [1].y = c->B.y + a->y;
	p [2].x = c->C.x + a->x; p [2].y = c->C.y + a->y;
	SDLERRNZ	(SDL_RenderDrawLines (Renderer, p, 3));
}
bool LegThings (int, int, unsigned short);
bool ReLeg (int x, int y, unsigned short leg) {
	/*if (leg >= 4)
		Leg [leg].x = x + 50;
	else
		Leg [leg].x = x - 50;
	Leg [leg].y = y + (leg % 5) * 16;*/
	
	return false; //LegThings (x, y, leg);
}

bool LegThings (int x, int y, unsigned short leg) {

	Point A = Leg [leg];
	
	Point C;
	C.x = x - A.x;
	C.y = y - A.y;
	
	Triangle test;
	test.a = 50;
	test.c = 50;
	test.b = SQRT (ABS (POW (C.x, 2)) + ABS (POW (C.y, 2)));
	
	if (test.b >= test.a + test.c)
		return ReLeg (x, y, leg);
	
	SolveTriangle_ABC (&test);
	
	if (isnan (test.A) || isnan (test.B) || isnan (test.C))
		return false;
	
	TriangleCoord coord;
	GetTriangleCoord (&C, &test, &coord, leg < 4 ? -1 : 1);
	
	/*for (unsigned short i = 0; i != 8; i ++) {
		if (i == leg)
			continue;
		if (Leg [i].x == Leg [leg].x && Leg [i].y == Leg [leg].y)
			return ReLeg (x, y, leg);
	}*/
	DrawTriangle (&coord, &A);
	return false;
}

void DoThings (void) {
	static float px = 0;
	static float py = 0;
	
	px -= (px - ((float) MouseX)) / 10;
	py -= (py - ((float) MouseY)) / 10;
	
	//SDLERRNZ (SDL_SetRenderDrawColor (Renderer, 0,0,0, 0));
	SDLERRNZ	(SDL_RenderClear (Renderer));
	SDLERRNZ	(SDL_RenderCopy (Renderer, BackgroundTexture, NULL, NULL));
	
	SDLERRNZ	(SDL_SetRenderDrawColor (Renderer, 255, 255, 255, 0));
	for (unsigned short i = 0; i != 8; i ++)
		if (LegThings (px, py, i))
			break;
	SDL_RenderPresent (Renderer);
}

void WaitFrame (void) {
	static Time last;
	Time target = last + FRAME_TIME;
	Time now = SDL_GetTicks ();
	
	if (target > now)
		SDL_Delay (target - now);
	last = target;
}

int main (void) {
	SDLERRNZ	(SDL_Init (SDL_INIT_VIDEO));
	SDL_CreateWindowAndRenderer (
		640, 480,
		0,
		&Window, &Renderer
	);
	
	SDLERRNULL	(Window);
	SDLERRNULL	(Renderer);
	
	BackgroundSurface = SDL_LoadBMP ("bkg.bmp");
	SDLERRNULL	(BackgroundSurface);
	
	BackgroundTexture = SDL_CreateTextureFromSurface (Renderer, BackgroundSurface);
	SDLERRNULL	(BackgroundTexture);
	
	SDLERRNZ	(SDL_RenderClear (Renderer));
	SDL_RenderPresent (Renderer);
	SDLERRNZ	(SDL_SetRenderDrawColor (Renderer, 255, 255, 255, 0));
	
	while (1) {
		while (SDL_PollEvent (&Event)) {
			switch (Event.type) {
				case SDL_QUIT:
					return 0;
				case SDL_MOUSEMOTION:
					MouseX = Event.motion.x;
					MouseY = Event.motion.y;
			}
		}
		DoThings ();
		WaitFrame ();
	}

}
