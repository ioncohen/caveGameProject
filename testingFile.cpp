#include <SDL.h>
#include <stdio.h>
#include <iostream>


int maain(int argc, char* args[]) {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("Cave Game Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 100, 100, SDL_WINDOW_SHOWN);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	SDL_Vertex verts[4];
	verts[0] = { {20,20},{0, 0, 0, 255},{1,1} };
	verts[1] = { {40,20}, {0, 0, 0, 255}, {1,1} };
	verts[2] = { {20,40}, {255, 255, 255, 255}, {1,1} };
	verts[3] = { {40,40}, {255, 255, 255, 255}, {1,1} };

	int indices[6] = {0,1,2,1,2,3};
	
	while (SDL_GetTicks() < 10000) {
		SDL_RenderGeometry(renderer, NULL, verts, 4, indices, 6);
		SDL_RenderPresent(renderer);
	}
	return 0;
}