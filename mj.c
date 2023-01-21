#include <SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

#define SIN sinf
#define COS cosf
#define TAN tanf
#define ASIN asinf
#define ACOS acosf
#define ATAN atanf

#define ATAN2 atan2f
#define POW powf
#define SQRT sqrtf
#define ABS fabsf


#define FRAME_TIME (1000 / 60)
#define LEGSZ	64

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


SDL_Renderer *Renderer = NULL;
SDL_Window *Window = NULL;
SDL_Surface *BackgroundSurface = NULL, *AttachSurface = NULL, *WindowSurface = NULL;
SDL_Texture *BackgroundTexture = NULL, *CircleTexture = NULL;
SDL_Event Event;

int MouseX, MouseY;
SDL_Rect Viewport;
struct _Leg {
	Coord a;
	Triangle last;
	Coord last_a;
	bool f;
} Leg [9];

bool *AttachTable = NULL;

void GenAttachTable (void) {
	if (AttachTable != NULL)
		free (AttachTable);
	AttachTable = malloc (sizeof (AttachTable [0]) * Viewport.w * Viewport.h + 1);
	
	SDL_Surface *surface;
	SDLERRNULL	(surface = SDL_ConvertSurfaceFormat (BackgroundSurface, SDL_PIXELFORMAT_RGB565, 0));
	
	SDLERRNZ	(SDL_LockSurface (surface));
	for (int y = 0; y != Viewport.h; y ++) {
		for (int x = 0; x != Viewport.w; x ++) {
			AttachTable [y * Viewport.w + x] = *(
				(uint8_t *) surface->pixels +
				y * surface->pitch +
				x * surface->format->BytesPerPixel
			) != 0;
		}
	}
	SDL_UnlockSurface (surface);
	
	SDL_FreeSurface (surface);
}

SDL_Texture *GenCircle (int radius, int res) {
	SDL_Surface *surface;
	SDL_Renderer *renderer;
	SDL_Texture *out;
	
	SDLERRNULL	(surface = SDL_CreateRGBSurfaceWithFormat (0, radius*2, radius*2, 8, SDL_PIXELFORMAT_ARGB32));
	SDLERRNULL	(renderer = SDL_CreateSoftwareRenderer (surface));
	
	SDLERRNZ	(SDL_SetRenderDrawColor (renderer, 0,0,0, 0));
	SDL_RenderClear (renderer);
	
	SDL_Point *l = malloc (sizeof (SDL_Point) * (res + 1));
	Float step = (2*M_PI)/res;
	for (int i = 0; i != res + 1; i ++) {
		l [i].x = COS (i * step) * radius + radius;
		l [i].y = SIN (i * step) * radius + radius;
	}
	
	SDLERRNZ	(SDL_SetRenderDrawColor (renderer, 255,255,255, 255));
	SDLERRNZ	(SDL_RenderDrawLines (renderer, l, res+1));
	free (l);
	
	SDL_RenderPresent (renderer);
	
	SDLERRNULL	(out = SDL_CreateTextureFromSurface (Renderer, surface));
	SDL_DestroyRenderer (renderer);
	SDL_FreeSurface (surface);
	
	return out;
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

bool ReLeg (int px, int py, short leg) {
	#define check	{\
		if ((ox+x) >= 0 && (ox+x) <= Viewport.w && (oy+y) >= 0 && (oy+y) <= Viewport.h) {\
			if (AttachTable [(oy+y) * Viewport.w + (ox+x)]) {\
				unsigned int dist = pow (ox, 2) + pow (oy, 2);\
				if (dist < closest) {\
					closest = dist;\
					closestat.x = ox + x;\
					closestat.y = oy + y;\
				}\
			}\
		}\
	}
	const int ofsx = (leg % 4) * (LEGSZ/16) + LEGSZ*1.5;
	const int ofsy = (leg % 4) * (LEGSZ/4);
	int x, y;
	if (leg < 4)
		x = px - ofsx;
	else
		x = px + ofsx;
	y = py + ofsy;
	
	if (x < 0 || y < 0 || x > Viewport.w || y > Viewport.h)
		return true;
	
	// do things or whatever
	const int sz_max = LEGSZ * 2 * 2;
	const unsigned int max_dist = 2 * pow (LEGSZ * 2, 2);
	unsigned int closest = -1;
	Coord closestat;
	
	signed short dir = 1;
	int ox = 0;
	int oy = 0;
	for (int sz = 1; sz != sz_max; sz ++) {
		while (2 * dir * ox < sz) {
			check;
			ox += dir;
		}
		while (2 * dir * oy < sz) {
			check;
			oy += dir;
		}
		dir = -1 * dir;
	}
	
	if (closest > max_dist) {
	//	Leg [leg].a.x = leg < 4 ? ofsx : -ofsx;
	//	Leg [leg].a.y = ofsy;
	//	Leg [leg].f = true;
		return false;
	}
	
	//Leg [leg].f = false;
	Leg [leg].a = closestat;
	#undef check
	
	return true;
}

void LegThings (int x, int y, short leg) {
	bool ul = false;
	Coord A = Leg [leg].a;
	
	Coord C;
	C.x = x - A.x;
	C.y = y - A.y;
	
	Triangle t;
	t.a = LEGSZ;
	t.c = LEGSZ;
	t.b = SQRT (ABS (POW (C.x, 2)) + ABS (POW (C.y, 2)));
	
	if (t.b > t.a + t.c)
		ul = true;
	
	SolveTriangle_ABC (&t);
	
	if (isnan (t.A) || isnan (t.B) || isnan (t.C))
		ul = true;
	
	if (ul) {
		ReLeg (x, y, leg);
		t = Leg [leg].last;
		A = Leg [leg].last_a;
		C.x = x - A.x;
		C.y = y - A.y;
		
		SolveTriangle_ABC (&t);
		
		if (isnan (t.A) || isnan (t.B) || isnan (t.C))
			return;
	} else {
		Leg [leg].last = t;
		Leg [leg].last_a = A;
	}
	
	TriangleCoord coord;
	GetTriangleCoord (&C, &t, &coord, leg < 4 ? -1 : 1);
	
	for (unsigned short i = 0; i != 8; i ++) {
		if (i == leg)
			continue;
		if (Leg [i].a.x == Leg [leg].a.x && Leg [i].a.y == Leg [leg].a.y)
			ReLeg (x, y, leg);
	}
	DrawTriangle (&coord, &A);
}


void DoThings (void) {
	static float px = 0;
	static float py = 0;
	
	px -= (px - ((float) MouseX)) / 10.0;
	py -= (py - ((float) MouseY)) / 10.0;
	
	SDLERRNZ	(SDL_SetRenderDrawColor (Renderer, 0,0,0, 0));
	SDLERRNZ	(SDL_RenderClear (Renderer));
	SDLERRNZ	(SDL_RenderCopy (Renderer, BackgroundTexture, NULL, NULL));
	SDLERRNZ	(SDL_SetRenderDrawColor (Renderer, 255,255,255, 0));
	for (short i = 0; i != 8; i ++)
		LegThings (px, py, i);
	
	SDL_Rect r;
	r.x = px - LEGSZ/2;
	r.y = py - LEGSZ/2;
	r.w = LEGSZ;
	r.h = LEGSZ;
	SDLERRNZ	(SDL_RenderCopy (Renderer, CircleTexture, NULL, &r));
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


void ResetWindow (int w, int h) {
	SDL_RenderGetViewport (Renderer, &Viewport);
	WindowSurface = SDL_GetWindowSurface (Window);
	
	if (BackgroundSurface != NULL)
		SDL_FreeSurface (BackgroundSurface);
	{
		Uint32 fmt = SDL_GetWindowPixelFormat (Window);
		BackgroundSurface = SDL_CreateRGBSurfaceWithFormat (
			0,
			Viewport.w, Viewport.h,
			SDL_BITSPERPIXEL (fmt), fmt
		);
		SDLERRNULL	(BackgroundSurface);
		
		SDLERRNZ	(SDL_BlitScaled (BackgroundImage, NULL, BackgroundSurface, NULL));
	}
	GenAttachTable ();
	
	if (BackgroundTexture != NULL)
		SDL_DestroyTexture (BackgroundTexture);
	SDLERRNULL	(BackgroundTexture = SDL_CreateTextureFromSurface (Renderer, BackgroundSurface));
}

int main (void) {
	SDLERRNZ	(SDL_Init (SDL_INIT_VIDEO));
	SDL_CreateWindowAndRenderer (
		640, 480,
		SDL_WINDOW_RESIZABLE,
		&Window, &Renderer
	);
	
	SDLERRNULL	(Window);
	SDLERRNULL	(Renderer);
	
	SDLERRNULL (BackgroundImage = SDL_LoadBMP ("bkg.bmp"));
	CircleTexture = GenCircle (LEGSZ/2, 8);
	
	ResetWindow (640, 480);
	SDLERRNZ	(SDL_RenderClear (Renderer));
	SDL_RenderPresent (Renderer);
	SDLERRNZ	(SDL_SetRenderDrawColor (Renderer, 255, 255, 255, 0));
	while (1) {
		while (SDL_PollEvent (&Event)) {
			switch (Event.type) {
				case SDL_QUIT:
					return 0;
				break;
				
				case SDL_MOUSEMOTION:
					MouseX = Event.motion.x;
					MouseY = Event.motion.y;
				break;
				
				case SDL_WINDOWEVENT:
					if (Event.window.event == SDL_WINDOWEVENT_RESIZED)
						ResetWindow (Event.window.event.data1, Event.window.event.data2);
				break;
			}
		}
		DoThings ();
		WaitFrame ();
	}

}
