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
#define LEGSZ	32

#define THROWSDLERR	fprintf (stderr, "SDL error at %u: %s\n", (unsigned int) __LINE__, SDL_GetError ())

#define SDLERRNZ(func)	{\
	if ((func) != 0) {\
		THROWSDLERR;\
		exit (1);\
	}\
}

#define SDLERRNULL(func)	{\
	if ((func) == NULL) {\
		THROWSDLERR;\
		exit (1);\
	}\
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
SDL_Rect Viewport;
Coord Leg [9] = { 0 };


void GenAttachTable (void) {
	
	SDL_PixelFormat *fmt;
	SDLERRNULL	(fmt = SDL_AllocFormat (SDL_PIXELFORMAT_RGB332));
	
	SDL_Surface *img;
	SDLERRNULL	(img = SDL_ConvertSurface (BackgroundSurface, fmt, 0));
	
	AttachTo = malloc (sizeof (AttachTo [0]) * (Viewport.w+1) * (Viewport.h+1) + 1);
	
	SDLERRNZ	(SDL_LockSurface (img));
	
	#define pix(x,y)	((x) * Viewport.w + (y))
	#define check	{\
		if (ox >= 0 && ox < Viewport.w && oy >= 0 && oy < Viewport.h) {\
			if (*((uint8_t *) img->pixels + pix (ox, oy)) != 0) {\
				unsigned int dist = (ox * ox) + (oy * oy);\
				if (dist < closest) {\
					closest = dist;\
					closestat.x = ox;\
					closestat.y = oy;\
				}\
			}\
		}\
	}
	
	for (int px = 0; px != Viewport.w; px ++) {
		for (int py = 0; py != Viewport.h; py ++) {
			for (unsigned short leg = 0; leg != 8; leg ++) {
				int x, y;
				if (leg > 4)
					x = px - 64;
				else
					x = px + 64;
				y = py + ((py % 5) * 16);
				
				if (x < 0 || y < 0 || x > Viewport.w || y > Viewport.h)
					continue;
				
				// do things or whatever
				int sz_max = LEGSZ * 2 * 2;
				const unsigned int max_dist = pow (LEGSZ, 2);
				unsigned int closest = -1;
				Coord closestat;
				
				signed short dir = 1;
				int ox = 0;
				int oy = 0;
				for (int sz = 1; sz != sz_max; sz ++) {
					while (2 * ox * dir < sz) {
						check;
						ox += dir;
					}
					while (2 * oy * dir < sz) {
						check;
						oy += dir;
					}
					dir *= -1;
				}
				
				if (closest > max_dist)
					closestat.x = closestat.y = -1;
				
				AttachTo [pix (x, y)].l [leg] = closestat;
			}
		}
	}
	#undef pix
	#undef check
	
	SDL_UnlockSurface (img);
	SDL_FreeSurface (img);
	SDL_FreeFormat (fmt);
}

void SolveTriangle_ABC (Triangle *t) {
	t->A = ACOS ((POW (t->b,2) + POW (t->c,2) - POW (t->a,2)) / (2 * t->b * t->c));
	t->B = ACOS ((POW (t->a,2) + POW (t->c,2) - POW (t->b,2)) / (2 * t->a * t->c));
	t->C = ACOS ((POW (t->a,2) + POW (t->b,2) - POW (t->c,2)) / (2 * t->a * t->b));
}

void GetTriangleCoord (const Coord *C, const Triangle *t, TriangleCoord *c, signed short flip) {
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

void DrawTriangle (const TriangleCoord *c, const Coord *a) {
	SDL_Point p [3];
	p [0].x = c->A.x + a->x; p [0].y = c->A.y + a->y;
	p [1].x = c->B.x + a->x; p [1].y = c->B.y + a->y;
	p [2].x = c->C.x + a->x; p [2].y = c->C.y + a->y;
	SDLERRNZ	(SDL_RenderDrawLines (Renderer, p, 3));
}

bool ReLeg (int x, int y, short leg) {
	/*if (leg >= 4)
		Leg [leg].x = x + 50;
	else
		Leg [leg].x = x - 50;
	Leg [leg].y = y + (leg % 5) * 16;*/
	
	Leg [leg] = AttachTo [x * Viewport.w + y].l [leg];
	
	return false; //LegThings (x, y, leg);
}

bool LegThings (int x, int y, short leg) {

	Coord A = Leg [leg];
	
	Coord C;
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
	for (short i = 0; i != 8; i ++)
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
	
	SDL_RenderGetViewport (Renderer, &Viewport);
	
	{
		SDL_Surface *bkg = SDL_LoadBMP ("bkg.bmp");
		SDLERRNULL	(bkg);
		
		Uint32 fmt = SDL_GetWindowPixelFormat (Window);
		BackgroundSurface = SDL_CreateRGBSurfaceWithFormat (
			0,
			Viewport.w, Viewport.h,
			SDL_BITSPERPIXEL (fmt), fmt
		);
		SDLERRNULL	(BackgroundSurface);
		
		SDLERRNZ	(SDL_BlitScaled (bkg, NULL, BackgroundSurface, NULL));
		
		SDL_FreeSurface (bkg);
	}
	GenAttachTable ();
	
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
