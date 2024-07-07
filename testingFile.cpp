#include <SDL.h>
#include <stdio.h>
#include <iostream>


int maain(int argc, char* args[]) {
	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window* window = SDL_CreateWindow("TEST WINDOW", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 500, 500, SDL_WINDOW_SHOWN);
	
	//SDL_Surface* windowSurface = SDL_GetWindowSurface(window);
	
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	SDL_Surface* tempSurface = SDL_CreateRGBSurfaceWithFormat(0, 500, 500, 32, SDL_PIXELFORMAT_RGBA8888);
	
	SDL_Rect rectangle;
	rectangle.x = 0;
	rectangle.y = 0;
	rectangle.w = 1;
	rectangle.h = 1;

	for (int i = 0; i < 500; i++) {
		for (int j = 0; j < 500; j++) {
			float xDist = (500 / 2 - i);
			float yDist = (500 / 2 - j);
			float dSquared = xDist * xDist + yDist * yDist;
			dSquared /= 100;
			dSquared++;
			//if (dSquared < 1) { dSquared = 1; };
			rectangle.x = i;
			rectangle.y = j;
			Uint32 color = (int)(255/ dSquared) | (int)(255/ dSquared) << 8 | (int)(255/ dSquared) << 16 | (int)(255/ dSquared) << 24;
			SDL_FillRect(tempSurface, &rectangle, color );
		}
	}
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, tempSurface);

	SDL_FreeSurface(tempSurface);
	//SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_ADD);


	//SDL_FillRect(tempSurface, &rectangle, UINT32_MAX);
	int i = 0;
	while ( i < 30000) {
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
		i++;
		printf("one loop done");
	}
	printf("we got to the end of the loop");
	return 0;
}