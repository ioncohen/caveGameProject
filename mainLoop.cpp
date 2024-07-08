#include <SDL.h>
#include <stdio.h>
#include "SimplexNoise.h"
#include <ctime>
#include <cstdlib>

#define PI 3.14159265

//THINGS TODO:
//		fix input direction
//		spiral lighting function
//		Do some subpixel stuff to mke slow movement smoother. Not sure how, either draw everything with sprites and floats (but store the same) or maybe store higher resolution terrain, and do smaller steps while displaying or something.
//			or both
//		
// 
// have the properties of the cave change with depth. then at the top have it spit out into just the ocean.
//		Cave miner? add ore pockets to collect and/or enemies/monsters to fight/chase you somehow (by following your trails?)
//		maybe different biomes where we blend two noise functions together?
//		

bool debugFlag = 0;

SDL_Color textColor = { 255, 255, 255, 255 };

const int SCREEN_WIDTH = 1010;
const int SCREEN_HEIGHT = 1010;

const int GAME_WIDTH = 100;
const int GAME_HEIGHT = 100;

const int MAX_DIM = GAME_WIDTH - (GAME_WIDTH - GAME_HEIGHT) * (GAME_HEIGHT > GAME_WIDTH);

const float ACCEL_RATE = 0.001;

//The window we'll be rendering to
SDL_Window* window = NULL;

//The surface contained by the window
SDL_Surface* windowSurface = NULL;

SDL_Surface* layerTwo;

//Game constants
bool caveTerrain[GAME_WIDTH][GAME_HEIGHT] = { {} };
float playerX = 0;
float playerY = 0;
float xSpeed = 0;       
float ySpeed = 0;
float xAccel = 0;
float yAccel = 0;

float noiseScaleFactor = 50;
float noiseCutoffLevel = 0;


int bufferOffsetX = 0;
int bufferOffsetY = 0;

int subPixelOffsetX = 0;
int subPixelOffsetY = 0;

//mod method that works better with negative numbers
int ringMod(int a, int b) {
	if (a >= 0) {
		return a % b;
	} else {
		return b + a % b;
	}
}

//method to set a pixel
void set_pixel(SDL_Surface* surface, int x, int y, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
	Uint32 pixel = blue | ((Uint32)green << 8) | ((Uint32)red << 16) | ((Uint32)alpha << 24);
	Uint8* target_pixel = (Uint8*)surface->pixels + y * surface->pitch + x * 4;
	*(Uint32*)target_pixel = pixel;
}

//pixel size constant
int pixelSize = 10;
int pixThick = 1;

SDL_Rect block;

void setBigPixel(int x, int y, uint8_t red, uint8_t green, uint8_t blue) {
	x = x*pixelSize - subPixelOffsetX;
	y = y*pixelSize - subPixelOffsetY;

	block.x = x;
	block.y = y;

	block.h = pixelSize;
	block.w = pixelSize;
	//blue version
	//Uint32 pixel = (blue/2) | ((Uint32)(green/3) << 8) | ((Uint32)0 << 16) | ((Uint32)255 << 24);
	Uint32 pixel = blue | ((Uint32)green << 8) | ((Uint32)red << 16) | ((Uint32)255 << 24);
	SDL_FillRect(windowSurface, &block, pixel);
}

void setBigPixelNoOffset(int x, int y, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
	x = x * pixelSize;
	y = y * pixelSize;

	block.x = x;
	block.y = y;

	block.h = pixelSize;
	block.w = pixelSize;
	//blue version
	//Uint32 pixel = (blue/2) | ((Uint32)(green/3) << 8) | ((Uint32)0 << 16) | ((Uint32)255 << 24);
	Uint32 pixel = blue | ((Uint32)green << 8) | ((Uint32)red << 16) | ((Uint32)alpha << 24);
	SDL_FillRect(windowSurface, &block, pixel);
}

void setBigPixelNoOffset(int x, int y, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha, SDL_Surface* layer) {
	x = x * pixelSize;
	y = y * pixelSize;

	block.x = x;
	block.y = y;

	block.h = pixelSize;
	block.w = pixelSize;
	//blue version
	//Uint32 pixel = (blue/2) | ((Uint32)(green/3) << 8) | ((Uint32)0 << 16) | ((Uint32)255 << 24);
	Uint32 pixel = blue | ((Uint32)green << 8) | ((Uint32)red << 16) | ((Uint32)alpha << 24);
	SDL_FillRect(layer, &block, pixel);
}

void setBigPixel(int x, int y, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha, SDL_Surface* layer) {
	x = x * pixelSize - subPixelOffsetX;
	y = y * pixelSize - subPixelOffsetY;

	block.x = x;
	block.y = y;

	block.h = pixelSize;
	block.w = pixelSize;
	//blue version
	//Uint32 pixel = (blue/2) | ((Uint32)(green/3) << 8) | ((Uint32)0 << 16) | ((Uint32)255 << 24);
	Uint32 pixel = blue | ((Uint32)green << 8) | ((Uint32)red << 16) | ((Uint32)255 << 24);
	SDL_FillRect(layer, &block, pixel);
}

void setBigPixel(int x, int y, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
	x = x * pixelSize - subPixelOffsetX;
	y = y * pixelSize - subPixelOffsetY;

	block.x = x;
	block.y = y;

	block.h = pixelSize;
	block.w = pixelSize;
	//blue version
	//Uint32 pixel = (blue/2) | ((Uint32)(green/3) << 8) | ((Uint32)0 << 16) | ((Uint32)255 << 24);
	Uint32 pixel = blue | ((Uint32)green << 8) | ((Uint32)red << 16) | ((Uint32)alpha << 24);
	SDL_FillRect(windowSurface, &block, pixel);
}

void setVerticalSlab(int x, int y, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha, SDL_Surface* layer) {
	if (!subPixelOffsetX) {
		return;
	}
	x = x * pixelSize + (pixelSize - subPixelOffsetX);
	y = y * pixelSize;

	block.x = x;
	block.y = y;

	block.h = pixelSize;
	block.w = subPixelOffsetX;

	Uint32 pixel = blue | ((Uint32)green << 8) | ((Uint32)red << 16) | ((Uint32)alpha << 24);
	SDL_FillRect(windowSurface, &block, pixel);
}

void setHorizontalSlab(int x, int y, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha, SDL_Surface *layer) {
	if (!subPixelOffsetY) {
		return;
	}
	x = x * pixelSize;
	y = y * pixelSize + (pixelSize - subPixelOffsetY);

	block.x = x;
	block.y = y;

	block.h = subPixelOffsetY;
	block.w = pixelSize;

	Uint32 pixel = blue | ((Uint32)green << 8) | ((Uint32)red << 16) | ((Uint32)alpha << 24);
	SDL_FillRect(windowSurface, &block, pixel);
}

bool OOB(int x, int y) {
	if (x > 0  && y > 0 && x < GAME_WIDTH && y < GAME_HEIGHT) {
		return false;
	}
	return true;
}

bool yOOB(int y) {
	return y < 0 || y >= GAME_HEIGHT;
}

bool xOOB(int x) {
	return x < 0 || x >= GAME_WIDTH;
}

float pixDistLookupTable[2 * GAME_WIDTH][2 * GAME_HEIGHT];
float pixDistLookupTable2[2 * GAME_WIDTH][2 * GAME_HEIGHT];

int occluded[GAME_WIDTH][GAME_HEIGHT] = { {} };

int pixTraceBackDirX[2 * GAME_WIDTH][2 * GAME_HEIGHT] = { {} };
int pixTraceBackDirY[2 * GAME_WIDTH][2 * GAME_HEIGHT] = { {} };

int pixTraceBackDirX2[2 * GAME_WIDTH][2 * GAME_HEIGHT] = { {} };
int pixTraceBackDirY2[2 * GAME_WIDTH][2 * GAME_HEIGHT] = { {} };

//function to fill a lookup table of distances for pixels of certain distances from each other
//useful for fast lighting loops
void fillLookupTables() {
	//Should we fill in with dist from point to center, or the other way around? bro its the same smh
	//the center of the array is GAME_WIDTH, GAME_HEIGHT I believe. all values are calced based on that.
	for (int i = 0; i < GAME_WIDTH << 1; i++) {
		for (int j = 0; j < GAME_HEIGHT << 1; j++) {
			int xdist = GAME_WIDTH - i;
			int ydist = GAME_HEIGHT - j;
			pixDistLookupTable[i][j] = sqrt(xdist*xdist + ydist*ydist);
			pixDistLookupTable2[i][j] = (xdist * xdist) + (ydist * ydist);
			
			if (i != GAME_WIDTH || j != GAME_HEIGHT) {
				float angleToCenter = atan2f(ydist, xdist) * 180 / 3.1415;
				if (angleToCenter < 0) { angleToCenter += 360; }
				int snapAngle = round(angleToCenter / (float)45) * 45;
				switch (snapAngle) {
				case 0:
					pixTraceBackDirX2[i][j] = 1;
					pixTraceBackDirY2[i][j] = 0;
					break;
				case 45:
					pixTraceBackDirX2[i][j] = 1;
					pixTraceBackDirY2[i][j] = 1;
					break;
				case 90:
					pixTraceBackDirX2[i][j] = 0;
					pixTraceBackDirY2[i][j] = 1;
					break;
				case 135:
					pixTraceBackDirX2[i][j] = -1;
					pixTraceBackDirY2[i][j] = 1;
					break;
				case 180:
					pixTraceBackDirX2[i][j] = -1;
					pixTraceBackDirY2[i][j] = 0;
					break;
				case 225:
					pixTraceBackDirX2[i][j] = -1;
					pixTraceBackDirY2[i][j] = -1;
					break;
				case 270:
					pixTraceBackDirX2[i][j] = 0;
					pixTraceBackDirY2[i][j] = -1;
					break;
				case 315:
					pixTraceBackDirX2[i][j] = 1;
					pixTraceBackDirY2[i][j] = -1;
					break;
				case 360:
					pixTraceBackDirX2[i][j] = 1;
					pixTraceBackDirY2[i][j] = 0;
					break;
				default:
					printf("ERROR:ANGLE ROUNDING");
				}
			}
			else {
				pixTraceBackDirX2[i][j] = 0;
				pixTraceBackDirY2[i][j] = 0;
			}
		}
	}
}

                     //first octant        //second(both pos, flip)   // third(x negative, same pairs)		//fourth(firstButxneg)					//fifth(bothneg,sameptasfirst)    //6th(2nd,2neg)                      //7th,						  /8th!
float rises[] = { 0,1,1,1,2,1,3,1,2,3,4,   1,2,3,3,4,4,5,5,5,5,       2, 3, 3, 4, 4, 5, 5, 5, 5,              0, 1, 1, 1, 2, 1, 3, 1, 2, 3, 4,       -1,-1,-1,-2,-1,-3,-1,-2,-3,-4,     -1,-2,-3,-3,-4,-4,-5,-5,-5,-5,      -2,-3,-3,-4,-4,-5,-5,-5,-5,    -1,-1,-1,-2,-1,-3,-1,-2,-3,-4,-0 };
float runs[] = { 1,1,2,3,3,4,4,5,5,5,5,    0,1,1,2,1,3,1,2,3,4,      -1,-1,-2,-1,-3,-1,-2,-3,-4,             -1,-1,-2,-3,-3,-4,-4,-5,-5,-5,-5,       -1,-2,-3,-3,-4,-4,-5,-5,-5,-5,     0,-1,-1,-2,-1,-3,-1,-2,-3,-4,        1, 1, 2, 1, 3, 1, 2, 3, 4,     1, 2, 3, 3, 4, 4, 5, 5, 5, 5, 1 };
const int numVirtualRays = 81;

//float rises[25] = { 0, 1, 1, 1, 2, 4, 1,  4,  2,  1,  1,  1,  0, -1, -1, -1, -2, -4, -1, -4, -2, -1 ,-1 ,-1, 0 };
//float runs[25] = { 1, 4, 2, 1, 1, 1, 0, -1, -1, -1, -2, -4, -1, -4, -2, -1, -1, -1,  0, 1,   1,  1,  2,  4, 1 };
float snapAngles[numVirtualRays];

//TODO: test if theres overshooting for pixels near the center. might have to edit those. not a big deal though i think.
void fillTraceBackTable() {
	printf("STARTING FILLTRACEBACKTABLE");
	for (int i = 0; i < numVirtualRays; i++) {
		snapAngles[i] = atan2f(rises[i], runs[i]) * 180 / 3.14159;
		printf("snapAngle %d: %f\n", i, snapAngles[i]);
		if (snapAngles[i] < 0 && snapAngles[i] > -2) { snapAngles[i] = 0; }
		if (snapAngles[i] < 0) { snapAngles[i] += 360; }
	}
	snapAngles[numVirtualRays] = 360;
	for (int i = 0; i < GAME_WIDTH << 1; i++) {
		for (int j = 0; j < GAME_HEIGHT << 1; j++) {
			int xdist = GAME_WIDTH - i;
			int ydist = GAME_HEIGHT - j;

			if (i != GAME_WIDTH || j != GAME_HEIGHT) {
				float angleToCenter = atan2f(ydist, xdist) * 180 / 3.14159;
				if (angleToCenter < 0) { angleToCenter += 360; }
				float closestAngle = 0;
				int closestAngleIndex = 0;
				float closestDistance = 365;
			
				for (int k = 0; k < numVirtualRays; k++) {
					if (abs(snapAngles[k] - angleToCenter) < closestDistance) {
						closestAngle = snapAngles[k];
						closestAngleIndex = k;
						closestDistance = abs(snapAngles[k] - angleToCenter);
					}
				}
				
				pixTraceBackDirX[i][j] = runs[closestAngleIndex];
				pixTraceBackDirY[i][j] = rises[closestAngleIndex];
				
			}
			else {
				pixTraceBackDirX[i][j] = 0;
				pixTraceBackDirY[i][j] = 0;
			}
		}
	}
	printf("FINISHING TRACEBACKFILL");
}

int clamp(int value) {
	return value * (value >= 0);
}

//all inputs are locations in the game grid. need to be translated wrt center
void handlePixelInSpiral(int x, int y, int spiralCenterX, int spiralCenterY) {
	
	/*DEBUG
	if (caveTerrain[ringMod(x + bufferOffsetX, GAME_WIDTH)][ringMod(y + bufferOffsetY, GAME_HEIGHT)]) {
		setBigPixel (x, y, 255, 255, 255);
	}
	return;
	//setBigPixel(x, y, pixDistLookupTable[x][y], pixDistLookupTable[x][y], pixDistLookupTable[x][y]);
	//return;
	//printf("x,y = (%d,%d)\n", x, y);
	*/
	
	int look1X = pixTraceBackDirX[x - spiralCenterX + GAME_WIDTH][y - spiralCenterY + GAME_HEIGHT];
	int look1Y = pixTraceBackDirY[x - spiralCenterX + GAME_WIDTH][y - spiralCenterY + GAME_HEIGHT];
	
	//printf("lookDirs = (%d,%d)\n", look1X, look1Y);
	if (occluded[x + look1X][y + look1Y]) {
		occluded[x][y] = 1;
	}
	else {
		//now check secondary lookback? add later
		if (!caveTerrain[ringMod(x + bufferOffsetX, GAME_WIDTH)][ringMod(y + bufferOffsetY, GAME_HEIGHT)]) {
			occluded[x][y] = 1;
			int smallLookX = pixTraceBackDirX2[x - spiralCenterX + GAME_WIDTH][y - spiralCenterY + GAME_HEIGHT];
			int smallLookY = pixTraceBackDirX2[x - spiralCenterX + GAME_WIDTH][y - spiralCenterY + GAME_HEIGHT];
			if (!caveTerrain[ringMod(x + smallLookX + bufferOffsetX, GAME_WIDTH)][ringMod(y + bufferOffsetY + smallLookY, GAME_HEIGHT)]) {
				setBigPixel(x, y, 0, 0, 0);	
			}
			else {
				setBigPixel(x, y, 255, 255, 255);
			}
			}
		else {
			occluded[x][y] = 0;
			int  dist = (int)pixDistLookupTable[x - spiralCenterX + GAME_WIDTH][y - spiralCenterY + GAME_HEIGHT];
			//printf("(%d,%d)\n", x,y);
			//printf("(%d)\n",dist);
			setBigPixel(x, y, clamp(255 - 3*dist), clamp(255 - 3 * dist), clamp(255 - 3 * dist));
		}
	}
}

//idea: optimize the spiral by checking if everything is occluded for 4 legs in a row. use ring buffer? or one int, edit with bitwise.
//TODO: edit drawpixel to clamp inputs?

const int numRays = 700;
void rayBasedLighting(int lightSourceX, int lightSourceY) {
	//should i do one ray at a time? or all at once?
	//one
	for (int i = 0; i < numRays; i++) {
		float x = lightSourceX;
		float y = lightSourceY;
		
		float xStep = cos(2 * PI * i / numRays);
		float yStep = sin(2 * PI * i / numRays);
		float distFromCenter = 1;
		float distFromCenter1 = 1;
		while ( distFromCenter < 50 && !OOB((int)x,(int)y) && caveTerrain[ringMod((int)(x)+bufferOffsetX,GAME_WIDTH)][ringMod((int)(y) + bufferOffsetY,GAME_HEIGHT)]) {
			setBigPixelNoOffset((int)x, (int)y, (int) 255/(distFromCenter), (int) 255 / (distFromCenter), (int) 255 / (distFromCenter), 100);
			x += xStep;
			y += yStep;
			distFromCenter = pixDistLookupTable2[(int)x - lightSourceX + GAME_WIDTH][(int)y - lightSourceY + GAME_HEIGHT]/100 + 1;
			distFromCenter1 = pixDistLookupTable[(int)x - lightSourceX + GAME_WIDTH][(int)y - lightSourceY + GAME_HEIGHT] / 20 + 1;
		}
		if (!OOB((int)x, (int)y) && distFromCenter < 50) {
			// (xStep < 0 || yStep < 0) {
				//setBigPixelNoOffset((int)x, (int)y, (int)255 / (distFromCenter), (int)255 / (distFromCenter), (int)255 / (distFromCenter), 0);
			//}
			if (debugFlag && xStep < 0) {
				setVerticalSlab((int)x, (int)y, (uint8_t)(255 / distFromCenter), (uint8_t)(255 / distFromCenter), (uint8_t)(255 / distFromCenter), 255, windowSurface);
			}
			if (debugFlag && yStep < 0) {
				setHorizontalSlab((int)x, (int)y, (uint8_t)(255 / distFromCenter), (uint8_t)(255 / distFromCenter), (uint8_t)(255 / distFromCenter), 255, windowSurface);

			}
			setBigPixel((int)x, (int)y, (uint8_t) (255 / distFromCenter1), (uint8_t)(255 / distFromCenter1), (uint8_t)(255 / distFromCenter1), 255, layerTwo);
		}
	}
}

void rayLightingHack(int lightSourceX, int lightSourceY) {
	for (int i = 0; i < numRays; i++) {
		float x = lightSourceX;
		float y = lightSourceY;

		float xStep = cos(2 * PI * i / numRays);
		float yStep = sin(2 * PI * i / numRays);
		int countSteps = 0;
		while (countSteps < 50 && !OOB((int)x, (int)y) && caveTerrain[ringMod((int)(x)+bufferOffsetX, GAME_WIDTH)][ringMod((int)(y)+bufferOffsetY, GAME_HEIGHT)]) {
			setBigPixel((int)x, (int)y, 0, 0, 0, 0, layerTwo);
			x += xStep;
			y += yStep;
			countSteps++;
		}
		if (!OOB((int)x, (int)y) && countSteps < 50) {
			//change some 255s later to be based on distance
			setBigPixel((int)x, int(y), 255, 255, 255, 255, layerTwo);

			// previous wall light func, maybe reuse for this^
			//setBigPixel((int)x, (int)y, (uint8_t)(255 / distFromCenter1), (uint8_t)(255 / distFromCenter1), (uint8_t)(255 / distFromCenter1), 255, layerTwo);
		}
	}
}

void handleLighting(int lightSourceX, int lightSourceY) {
	//*******idea, do more checking in outer loop so no checking necessary in inner loop.
	//calculate beforehand where the inbounds portion of the loop will start and end.
	//Then the inner loop is just a for loop with fixed endpoints, no checking.

	//we have to spiral around, and we have to deal with a non-square screen
	//sequence we want: [(0,0),(1,0),(1,1),(0,1),(-1,1),(-1,0),(-1,-1),(0,-1),(1,-1),(2,-1),(,),(,),(,),(,),(,),(,)]

	for (int i = 0; i < GAME_WIDTH; i++)
		for (int j = 0; j < GAME_HEIGHT; j++)
			occluded[lightSourceX + i][lightSourceY + j] = 0;
	
	int x = lightSourceX;
	int y = lightSourceY;
	bool movingX = 1;
	int signOfMove = 1;
	int numDisplayed = 0;
	int legLength = 1;
	int caseCount = 0;

	while (numDisplayed < GAME_HEIGHT*GAME_WIDTH) {
		int movesToGo = legLength;
		if (movingX && yOOB(y)) {
			x += movesToGo * signOfMove;
			movesToGo = 0;
		} 
		else if (!movingX && xOOB(x)) {
			y += movesToGo * signOfMove;
			movesToGo = 0;
		} 
		else if (movingX && signOfMove == 1 && x < 0) {
			//skip to x being in bounds
			movesToGo += x;
			x = 0;
		}
		else if (movingX && signOfMove == -1 && x >= GAME_WIDTH) {
			movesToGo -= x - GAME_WIDTH + 1;
			x = GAME_WIDTH - 1;
		}
		else if (!movingX && signOfMove == 1 && y < 0) {
			movesToGo += y;
			y = 0;
		}
		else if (!movingX && signOfMove == -1 && y >= GAME_HEIGHT) {
			movesToGo -= y - GAME_HEIGHT + 1;
			y = GAME_HEIGHT - 1;
		}

		//ok, now we are in bounds, and we just have to go until leglen is 0?
		if (movingX){
			while (movesToGo > 0) {
				if (xOOB(x)) {
					x += movesToGo * signOfMove;
					movesToGo = 0;
					continue;
				}
				//printf("(%d,%d)", x, y);
				//setBigPixel(x, y, legLength % 255, legLength % 255, legLength % 255, 255);
				//determine occlusion
				//find previous
				handlePixelInSpiral(x, y, lightSourceX, lightSourceY);


				numDisplayed++;
				x += signOfMove;
				movesToGo--;
				
			}
		}
		else {
			while (movesToGo > 0) {
				if (yOOB(y)) {
					y += movesToGo * signOfMove;
					movesToGo = 0;
					continue;
				}
				//printf("(%d,%d)", x, y);
				handlePixelInSpiral(x,y, lightSourceX, lightSourceY);

				//setBigPixel(x, y, (legLength/2 + ((numDisplayed) % 255) / 10) % 255, (legLength / 2 + ((numDisplayed) % 255) / 10) % 255, (legLength / 2 + (numDisplayed % 255) / 10) % 255, 255);
				numDisplayed++;

				y += signOfMove;
				movesToGo--;
			}
		}


		//finished a leg:
		movingX = !movingX;
		if (movingX == 1)
			signOfMove = -signOfMove;
		if (movingX == 1)
			legLength++;
	}
	//printf("finishedDisplaying the spiral :)!!!!");
}

//it might be time to split into multiple files
//TODO: change pixel drawing func to not create a new rect every time? just change the . check if atually improves performance though.	

void initializeEverything() {
	srand(time(0));

	//first, we initialize SDL
	SDL_Init(SDL_INIT_VIDEO);

	//create window
	window = SDL_CreateWindow("Cave Game Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

	//now get window surface
	windowSurface = SDL_GetWindowSurface(window);

	layerTwo = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_PIXELFORMAT_ARGB8888);

	SDL_SetSurfaceBlendMode(layerTwo, SDL_BLENDMODE_BLEND);

	//fill pixelDistance lookup table
	fillLookupTables();
	fillTraceBackTable();

	//initialize the terrain array:
	for (int i = 0; i < GAME_WIDTH; i++) {
		for (int j = 0; j < GAME_HEIGHT; j++) {
			caveTerrain[i][j] = SimplexNoise::noise(i / noiseScaleFactor, j / noiseScaleFactor) > noiseCutoffLevel;
		}
	}
}

int main(int argc, char* args[]) {
	
	int frameCount = 0;
	
	initializeEverything();

	//ok now we can start our main loop:
	bool quit = false;
	SDL_Event event;

	//bools to keep track of where/if we are accelerating
	bool pushingLeft = false;
	bool pushingUp = false;
	bool pushingRight = false;
	bool pushingDown = false;

	int prevBufferOffsetX = 0;
	int prevBufferOffsetY = 0;

	const int fpsWindowSize = 100;

	uint32_t prevTicks = -1000;
	uint32_t lastNFrames[fpsWindowSize] = {};
	
	while (!quit) {
		//first thing we do in the loop is handle inputs
		while (SDL_PollEvent(&event) != 0) {
			//ok so what events are we thinking of? x out, move mouse, shift and ctrl for height
			if (event.type == SDL_QUIT) {
				quit = true;
				break;
			}
			if (event.type == SDL_KEYDOWN) {
				switch (event.key.keysym.sym) {
					case SDLK_0:
						debugFlag = 1;
						break;

					case SDLK_UP:
						pushingUp = true;
						break;

					case SDLK_DOWN:
						pushingDown = true;
						break;

					case SDLK_LEFT:
						pushingLeft = true;
						break;

					case SDLK_RIGHT:
						pushingRight = true;
						break;

					default:
						break;
				}
			}
			if (event.type == SDL_KEYUP) {
				switch (event.key.keysym.sym) {
					case SDLK_0:
						debugFlag = false;
						break;
					case SDLK_UP:
						pushingUp = false;
						yAccel = 0;
						break;
					case SDLK_DOWN:
						pushingDown = false;
						yAccel = 0;
						break;
					case SDLK_LEFT:
						pushingLeft = false;
						xAccel = 0;
						break;
					case SDLK_RIGHT:
						pushingRight = false;
						xAccel = 0;
						break;
					default:
						break;
				}
			}
		}

		//now adjust movement speed
		bool pushingSide = pushingLeft || pushingRight;
		bool pushingVert = pushingUp || pushingDown;
		
		if (pushingLeft) {
			if (pushingVert) {
				xAccel = -ACCEL_RATE/sqrt(2);
			} else {
				xAccel = -ACCEL_RATE;
			}
		}
		if (pushingRight) {
			if (pushingVert) {
				xAccel = ACCEL_RATE / sqrt(2);
			}
			else {
				xAccel = ACCEL_RATE;
			}
		}
		if (pushingUp) {
			if (pushingSide) {
				yAccel = ACCEL_RATE / sqrt(2);
			} else {
				yAccel = ACCEL_RATE;
			}
		}
		if (pushingDown) {
			if (pushingSide) {
				yAccel = -ACCEL_RATE / sqrt(2);
			}
			else {
				yAccel = -ACCEL_RATE;
			}
		}

		lastNFrames[frameCount % fpsWindowSize] = SDL_GetTicks();
		//int avgFPS = fpsWindowSize/(lastNFrames[frameCount % fpsWindowSize] - lastNFrames[(frameCount + 1)%fpsWindowSize]);
		printf("FPS: % d\n", (fpsWindowSize*1000)/(lastNFrames[frameCount % fpsWindowSize] - lastNFrames[(frameCount + 1) % fpsWindowSize]));

		float frameDelta = 1;
		//float frameDelta = (SDL_GetTicks() - prevTicks)/2;
		//if (frameDelta <= 0) { frameDelta = 1; }
		//prevTicks = SDL_GetTicks();
		
		xSpeed += xAccel/frameDelta;
		ySpeed += yAccel/frameDelta;

		//TODO: figure out a way to frameDelta this
		if (xAccel == 0) {
			xSpeed *= 0.99;
		}
		if (yAccel == 0) {
			ySpeed *= 0.99;
		}

		/*if (xSpeed > 0.9)
			xSpeed = 0.9;
		if (ySpeed > 0.9)
			ySpeed = 0.9;
		*/

		int prevX = floor(playerX);
		int prevY = floor(playerY);

		playerX += xSpeed/frameDelta;
		playerY += ySpeed/frameDelta;

		//maybe we have to do it virtually.
		//ok dont use bufferoffset, just use discrete x and y
		
		int prevBufferOffsetX = bufferOffsetX;
		int prevBufferOffsetY = bufferOffsetY;

		//calculate new bufferOffset from player position
		//we use unsigned int to get the ring buffer modulus behavior, hopefully.
		bufferOffsetX = ringMod(floor(playerX),GAME_WIDTH);
		bufferOffsetY = ringMod(floor(playerY),GAME_HEIGHT);

		//ok now need buffer fill direction.
		int bufferDirectionX = (xSpeed >= 0) - (xSpeed < 0);
		int bufferDirectionY = (ySpeed >= 0) - (ySpeed < 0);
		
		int numNewLinesX = abs(floor(playerX) - prevX);
		int numNewLinesY = abs(floor(playerY) - prevY);

		//idea. Imaginary loop start. imagine we are filling in the whole array. wewould loop i and j from 0 to height, and then put the offset in the left and the x pos in the right!
		//ok now just start the loop at the lines we need to fill?
		//split into cases based on bufferdirection i think.
		int istart=0;
		int iend=0;

		if (bufferDirectionX == -1) {
			iend = numNewLinesX;
		} else {
			istart = GAME_WIDTH - numNewLinesX;
			iend = GAME_WIDTH;
		}
		for (int i = istart; i < iend; i++) {
			for (int j = 0; j < GAME_HEIGHT; j++) {
				caveTerrain[ringMod(i + bufferOffsetX, GAME_WIDTH)][ringMod(j + bufferOffsetY, GAME_HEIGHT)] = SimplexNoise::noise((i + playerX) / noiseScaleFactor, (j + playerY) / noiseScaleFactor) > noiseCutoffLevel;
			}
		}

		int jstart = 0;
		int jend = 0;

		if (bufferDirectionY == -1 ) {
			jend = numNewLinesY;
		}
		else {
			jstart = GAME_HEIGHT - numNewLinesY;
			jend = GAME_HEIGHT;
		}
		for (int i = 0; i < GAME_WIDTH; i++) {
			for (int j = jstart; j < jend; j++) {
				caveTerrain[ringMod(i + bufferOffsetX, GAME_WIDTH)][ringMod(j + bufferOffsetY, GAME_HEIGHT)] = SimplexNoise::noise((i + playerX) / noiseScaleFactor, (j + playerY) / noiseScaleFactor) > noiseCutoffLevel;
			}
		}

		//clear the screen so we can draw to it later
		SDL_FillRect(windowSurface, NULL, 0x000000);

		//calculate subpixel offset for frame drawing purposes
		float subPixelDiffX = playerX - floor(playerX);
		float subPixelDiffY = playerY - floor(playerY);
		
		subPixelOffsetX = round(subPixelDiffX * (pixelSize - 1));
		subPixelOffsetY = round(subPixelDiffY * (pixelSize - 1));

		//at this point our cave terrain array shoul dbe good. it has all the right data storred in the weird way in all the right places. So we just have to print it out using the buffer?
		//
		
		//Uint32 pixel =  0| ((Uint32)0 << 8) | ((Uint32)0 << 16) | ((Uint32)0 << 24);
		
		SDL_FillRect(layerTwo, NULL, 0);
		//handleLighting(GAME_WIDTH/2, GAME_HEIGHT/2);
		rayLightingHack(GAME_WIDTH /2, GAME_HEIGHT / 2);
		SDL_BlitSurface(lightingSurface, NULL, windowSurface, NULL);
		//rayBasedLighting(GAME_WIDTH / 4, GAME_HEIGHT / 2);
		//rayBasedLighting(2 * GAME_WIDTH / 3, 2 * GAME_HEIGHT / 3);
		SDL_BlitSurface(layerTwo, NULL, windowSurface, NULL);

		//setBigPixelNoOffset(GAME_WIDTH / 2, GAME_HEIGHT / 2, 255, 0, 0, 255);

		//test of spiral raycaster
		//draw50Lines
		
		/*
		for (int i = 0; i < GAME_WIDTH; i++) {
			for (int j = 0; j < GAME_HEIGHT; j++) {
				if (!caveTerrain[(i + bufferOffsetX) % GAME_WIDTH][(j + bufferOffsetY) % GAME_HEIGHT]) {
					if (i == 50 && j == 50) {
						setBigPixel(i, j, 0, 0, 255, 255);
					}
					else {
						setBigPixel(i, j, 255, 255, 255, 255);
					}
				}
			}
		}
		*/


		//update the surface, because we have made all the necessary changes
		SDL_UpdateWindowSurface(window);
		//printf("frame done");
		frameCount++;
		//printf("OVR AVG FPS: %d", 1000 * frameCount / SDL_GetTicks());
}

	//free up everything and quit
	//Destroy window
	SDL_DestroyWindow(window);
	window = NULL;

	SDL_FreeSurface(layerTwo);
	
	//Quit SDL subsystems
	SDL_Quit();

	return 0;
}
