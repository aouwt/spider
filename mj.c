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


typedef long double Float;
typedef Float TAngle;
typedef Float TSide;

typedef struct _Point {
	Float x, y;
} Point;

typedef struct _Triangle {
	TSide a, b, c;
	TAngle A, B, C;
} Triangle;

typedef struct _TriangleCoord {
	Point A, B, C;
} TriangleCoord;

SDL_Renderer *Renderer = NULL;
SDL_Window *Window = NULL;
SDL_Event Event;
Point Leg [9] = { 0 };

/*void SolveTriangle_ABC (Triangle *t) {
	// simplify (solve (a^2 = b^2 + c^2 - 2*b*c*cos (aa), aa)) -> (pi+2*asin((a^2-b^2-c^2)/(2*b*c)))/2
	t->A = (M_PI + 2.0 * ASIN ((POW (t->a, 2) - POW (t->b, 2) - POW (t->c, 2)) / (2.0 * t->a * t->c))) / 2.0;
	// simplify (solve ((a / sin (aa)) = (b / sin (ab)), ab)) -> asin(b*sin(A)/a)
	t->B = ASIN (t->b * SIN (t->A) / t->a);
	// simplify (solve (A + B + C = 180, C)) -> 180 - A - B
	t->C = 180.0 - (t->A + t->B);
}*/
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
	SDL_RenderDrawLines (Renderer, p, 3);
}
bool LegThings (int, int, unsigned short);
bool ReLeg (int x, int y, unsigned short leg) {
	if (leg >= 4)
		Leg [leg].x = x + 64;
	else
		Leg [leg].x = x - 64;
	Leg [leg].y = y + (leg % 5) * 16;
	return LegThings (x, y, leg);
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
	
	for (unsigned short i = 0; i != 8; i ++) {
		if (i == leg)
			continue;
		if (Leg [i].x == Leg [leg].x && Leg [i].y == Leg [leg].y)
			return ReLeg (x, y, leg);
	}
	DrawTriangle (&coord, &A);
	return false;
}

void DoThings (void) {
	static float px = 0;
	static float py = 0;
	
	px -= (px - ((float) Event.motion.x)) / 10;
	py -= (py - ((float) Event.motion.y)) / 10;
	
	SDL_SetRenderDrawColor (Renderer, 0, 0, 0, 0);
	SDL_RenderClear (Renderer);
	
	SDL_SetRenderDrawColor (Renderer, 255, 255, 255, 0);
	for (unsigned short i = 0; i != 8; i ++)
		if (LegThings (px, py, i))
			break;
	SDL_RenderPresent (Renderer);
}

int main (void) {
	SDL_Init (SDL_INIT_EVERYTHING);
	SDL_CreateWindowAndRenderer (
		640, 480,
		0,
		&Window, &Renderer
	);
	
	SDL_RenderClear (Renderer);
	SDL_RenderPresent (Renderer);
	SDL_SetRenderDrawColor (Renderer, 255, 255, 255, 0);
	
	while (1) {
		SDL_WaitEvent (&Event);
		switch (Event.type) {
			case SDL_QUIT:
				return 0;
			case SDL_MOUSEMOTION:
				DoThings ();
		}
	}

}
