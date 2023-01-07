a.out: mj.c
	$(CC) mj.c $(shell sdl2-config --cflags --libs) -lm -Wall -Wextra -Wpedantic $(CFLAGS)
