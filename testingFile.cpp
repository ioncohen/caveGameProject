#include <SDL.h>
#include <stdio.h>
#include <iostream>


int maain(int argc, char* args[]) {
	printf("C++ sin(2): %f\n", sin(2));
	printf("C++ sinf(2): %f\n", sinf(2));
	printf("SDL SDL_Sin(2): %f\n", SDL_sin(2));
	printf("SDL SDL_Sinf(2): %f\n", SDL_sinf(2));
	printf("");
	return 0;
}