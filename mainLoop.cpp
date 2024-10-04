#include <SDL.h>
#include <stdio.h>
#include "SimplexNoise.h"
#include <ctime>
#include <cstdlib>
#include <string.h>
#include <chrono>
#include <unordered_map>
#include <boost/functional/hash.hpp>

struct hashPair
{
	std::size_t operator() (const std::pair<int,int>& m) const
	{
		std::size_t s = 0;
		boost::hash_combine(s, m.first);
		boost::hash_combine(s, m.second);
		return s;
	}
};


#define PI 3.14159265

const float toRad = 0.01745329251;
const float toDeg = 57.2957795131;

//THINGS TODO:
//		fix input direction
//
// have the properties of the cave change with depth. then at the top have it spit out into just the ocean.
//		Cave miner? add ore pockets to collect and/or enemies/monsters to fight/chase you somehow (by following your trails?)
//		maybe different biomes where we blend two noise functions together?
//		
//		
// 
// 
//		

bool debugFlag = 0;

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

//Second layer that gets blitted onto the window
SDL_Surface* layerTwo = NULL;

//Renderer to draw textures from GPU
SDL_Renderer* renderer = NULL;

//Texture to store the lighting falloff. BLENDMODE BLEND
SDL_Texture* lightingTexture = NULL;

//Texture to store the Strong flashlight. BLENDMODE NONE
SDL_Texture* flashLightingTexture = NULL;

//Buffer for composing the lightmap
SDL_Texture* lightingBuffer = NULL;

//Black texture to poke holes in and renderclear
SDL_Texture* darknessTexture = NULL;

//player submarine texture
SDL_Texture* playerSubmarineTexture = NULL;

//submarine flashlight texture
SDL_Texture* flashlightTexture = NULL;

//submarine buffer texture
SDL_Texture* submarineBuffer = NULL;

//Final buffer before the actual screen
SDL_Texture* finalBuffer = NULL;

SDL_Texture* bubbleTexture = NULL;

//texture to store the bars
SDL_Texture* metersTexture = NULL;

//starry sky texture
SDL_Texture* starrySkyTexture = NULL;

//moon texture
SDL_Texture* moonTexture = NULL;

//Cloud texture
SDL_Texture* cloudTexture = NULL;

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


struct bubble {
	//pos
	float x;
	float y;

	//spd
	float xSpd;
	float ySpd;

	//time in milliseconds when the bubble pops
	Uint32 popTime;
};

//idea: width gets smaller before it gets larger, so theres small paths down into the caves instead of patches.
float getCaveWidth(float y, float x) {
	if (y < -400 + 20*SimplexNoise::noise(x/50)) { return 1; }
	float caveWidth = pow(1.0000000001, (-y*y*y*y)) - 0.4;
	
		//float caveWidth = 0.6 - y / 500;
	//if (caveWidth > 0.5) {
	//	caveWidth += 3 * (caveWidth - 0.5);
	//}
	return caveWidth;
}

//basically limits map size to 2 billion. is that ok? probably.
std::unordered_map<std::pair<int,int>, short, hashPair> explosions;
const int expGrain = 1;
const int expSize = 8;


//returns 1 if open water, 0 if cave?
bool caveNoise(float x, float y) {
	//generate the width modifier from y value.
	//return the noise check
	float explosionMod = 0;
	if (explosions.find(std::pair<int,int>(round(x / expGrain), round(y/ expGrain))) != explosions.end()) {
		explosionMod = 1;
		printf("explosion detected here!!!!");
	}
	return SimplexNoise::noise(x / noiseScaleFactor, y / noiseScaleFactor) + getCaveWidth(y , x) + explosionMod > noiseCutoffLevel;
}

//mod method that loops around for negative numbers
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

	//blue version
	//Uint32 pixel = (blue/2) | ((Uint32)(green/3) << 8) | ((Uint32)0 << 16) | ((Uint32)255 << 24);
	Uint32 pixel = blue | ((Uint32)green << 8) | ((Uint32)red << 16) | ((Uint32)alpha << 24);
	SDL_FillRect(windowSurface, &block, pixel);
}

void setBigPixelNoOffset(int x, int y, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha, SDL_Surface *layer) {
	x = x * pixelSize;
	y = y * pixelSize;

	block.x = x;
	block.y = y;

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
	//blue version
	//Uint32 pixel = (blue/2) | ((Uint32)(green/3) << 8) | ((Uint32)0 << 16) | ((Uint32)255 << 24);
	Uint32 pixel = blue | ((Uint32)green << 8) | ((Uint32)red << 16) | ((Uint32)alpha << 24);
	SDL_FillRect(layer, &block, pixel);
}

void setBigPixel(int x, int y, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
	x = x * pixelSize - subPixelOffsetX;
	y = y * pixelSize - subPixelOffsetY;

	block.x = x;
	block.y = y;

	//blue version
	//Uint32 pixel = (blue/2) | ((Uint32)(green/3) << 8) | ((Uint32)0 << 16) | ((Uint32)255 << 24);
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
		//printf("snapAngle %d: %f\n", i, snapAngles[i]);
		if (snapAngles[i] < 0 && snapAngles[i] > -2) { snapAngles[i] = 0; }
		if (snapAngles[i] < 0) { snapAngles[i] += 360; }
	}
	snapAngles[numVirtualRays-1] = 360;
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
 
//idea, use memcopy to fill occluded array, then propagate 1s out? or at least then you dont have to check
//maybe redo the pixtraceback arrays to be one big array that returns a tuple. That way dont have to calculate the indexes 4 times.
//how can we change the spiral to work with multiple light sources? it will already work with any light distnce.
//we can use additive lighting to generate the lightmap background. but how to calculate occlusion from many sources? i guess we just have to double our work :(
void rowByRowLighting(int lightSourceX, int lightSourceY, float mouseAngle) {
	memset(occluded, 0, sizeof occluded);

	SDL_SetRenderTarget(renderer, darknessTexture);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); //might be unnecessary
	SDL_RenderClear(renderer);

	//upper right (in my head) quadrant
	int xdir = 1;
	int ydir = 1;
	for (int q = 0; q < 4; q++) {
		for (int j = lightSourceY; j < GAME_HEIGHT && j >= 0; j += xdir) {
			for (int i = lightSourceX; i < GAME_WIDTH && i >= 0; i+= ydir) {
				int x = i + pixTraceBackDirX[i - lightSourceX + GAME_WIDTH][j - lightSourceY + GAME_HEIGHT];
				int y = j + pixTraceBackDirY[i - lightSourceX + GAME_WIDTH][j - lightSourceY + GAME_HEIGHT];
				//check if prev is occluded
				if (occluded[x][y]) {
					occluded[i][j] = 1;
				}
				else if (!caveTerrain[ringMod(i + bufferOffsetX, GAME_WIDTH)][ringMod(j + bufferOffsetY, GAME_HEIGHT)]) {
					occluded[i][j] = 1;
					block.x = i * pixelSize - subPixelOffsetX;
					block.y = j * pixelSize - subPixelOffsetY;
					int x2 = i + pixTraceBackDirX2[i - lightSourceX + GAME_WIDTH][j - lightSourceY + GAME_HEIGHT];
					int y2 = j + pixTraceBackDirY2[i - lightSourceX + GAME_WIDTH][j - lightSourceY + GAME_HEIGHT];
					if (!occluded[x2][y2]) {
						//in this case, we have hit a wall for the first time
						float dist = (i - lightSourceX) * (i - lightSourceX) + (j - lightSourceY) * (j - lightSourceY);
						dist /= 80;
						//dist += 1;
						
						//check if we are in the angle
						float pixAngle = atan2f(j - lightSourceY, i - lightSourceX)*toDeg;
						
						if (abs(pixAngle - mouseAngle) < 15 || abs(pixAngle + 360 - mouseAngle) < 15) {
							SDL_SetRenderDrawColor(renderer, 255/(dist/6+1), 255/ (dist/6 + 1), 255/ (dist/6 + 1), 255);
							SDL_RenderFillRect(renderer, &block);
						}
						else {
							SDL_SetRenderDrawColor(renderer, 100 / (dist + 1), 100 / (dist + 1), 100 / (dist + 1), 255);
							SDL_RenderFillRect(renderer, &block);
						}
					}
				}
				else {
					//prev in light, and we are not terrain. So we are in light. Draw a transparent block.
					block.x = i * pixelSize - subPixelOffsetX;
					block.y = j * pixelSize - subPixelOffsetY;
					SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
					SDL_RenderFillRect(renderer, &block);
				}
			}
		}
		xdir = -xdir;
		if (xdir == 1) {
			ydir = -ydir;
		}
	}
	SDL_SetRenderTarget(renderer, finalBuffer);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); //might be unnecessary
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
//Controls brightness of the lighting texture. Higher is brighter/farther

//const float distanceScaleFactor = 10; for 
SDL_Texture* createLightingTexture(const float distanceScaleFactor, SDL_BlendMode bMode, Uint8 alpha) {
	//TODO: allow tis method to be used from any pt, not just center.
	SDL_Surface* lightingSurface = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_PIXELFORMAT_RGBA8888); //maybe remove alpha channel?
	//SDL_SetSurfaceBlendMode(lightingSurface, SDL_BLENDMODE_ADD);
	for (int i = 0; i < GAME_WIDTH; i++) {
		for (int j = 0; j < GAME_HEIGHT; j++) {
			float xDist = (GAME_WIDTH / 2 - i);
			float yDist = (GAME_HEIGHT / 2 - j);
			float dSquared = xDist * xDist + yDist * yDist;
			dSquared /= distanceScaleFactor;
			dSquared++;
			//if (dSquared < 1) { dSquared = 1; };
			if (alpha == -1) {
				setBigPixelNoOffset((int)i, (int)j, (int)255 / (dSquared), (int)255 / (dSquared), (int)255 / (dSquared), (int)255 / (dSquared), lightingSurface);
			}
			else {
				setBigPixelNoOffset((int)i, (int)j, (int)255 / (dSquared), (int)255 / (dSquared), alpha, (int)255 / (dSquared), lightingSurface);
			}
		}
	}
	//Uint32 color = 255 | (uint32_t)255 << 8;
	//SDL_FillRect(lightingSurface, NULL, UINT32_MAX);


	printf("Making lighting texture of brightness %f\n", distanceScaleFactor);
	SDL_Texture* result = SDL_CreateTextureFromSurface(renderer, lightingSurface);
	
	if (result == NULL) {
		printf("ERROR: Lighting texture creation failed");
	}

	SDL_FreeSurface(lightingSurface);
	SDL_SetTextureBlendMode(result, bMode);

	return result;
}

const int numRays = 360;
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
			setBigPixel((int)x, (int)y, (uint8_t) (255 / distFromCenter1), (uint8_t)(255 / distFromCenter1), (uint8_t)(255 / distFromCenter1), 255, layerTwo);
		}
	}
}

//TODO: edit this function so that it marks pixels in layer2 as transparent, rather than drawing the same distance function forever.
//	Create a modified setpixel that just turns things to all 0s.
void rayLightingHack(int lightSourceX, int lightSourceY) {
	SDL_SetRenderTarget(renderer, darknessTexture);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); //might be unnecessary
	SDL_RenderClear(renderer);
	for (int i = 0; i < numRays; i++) {
		float x = lightSourceX;
		float y = lightSourceY;

		float xStep = cos(2 * PI * i / numRays);
		float yStep = sin(2 * PI * i / numRays);
		int countSteps = 0;
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		while (countSteps < 50 && !OOB((int)x, (int)y) && caveTerrain[ringMod((int)(x)+bufferOffsetX, GAME_WIDTH)][ringMod((int)(y)+bufferOffsetY, GAME_HEIGHT)]) {
			block.x = (int)x * pixelSize-subPixelOffsetX;
			block.y = (int)y * pixelSize-subPixelOffsetY;
			SDL_RenderFillRect(renderer, &block);
			x += xStep;
			y += yStep;
			countSteps++;
		}
		if (!OOB((int)x, (int)y) && countSteps < 50) {
			block.x = (int)x * pixelSize-subPixelOffsetX;
			block.y = (int)y * pixelSize-subPixelOffsetY;
			SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
			SDL_RenderFillRect(renderer, &block);
			
			// previous wall light func, maybe reuse for this^
			//setBigPixel((int)x, (int)y, (uint8_t)(255 / distFromCenter1), (uint8_t)(255 / distFromCenter1), (uint8_t)(255 / distFromCenter1), 255, layerTwo);
		}
	}
	SDL_SetRenderTarget(renderer, finalBuffer);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); //might be unnecessary
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

//Takes in world positions, returns a slope (in 45 steps) if you are colliding, 0 if not?
int marchingSlope(float x, float y) {
	//maybe change thid to use the cT buffer instead
	//abcd = bot left, bot right, top left, top right (in imaginary world)
	int ax = floor(x);
	int ay = floor(y);

	int bx = floor(x) + 1;
	int by = floor(y);

	int cx = floor(x);
	int cy = floor(y) + 1;

	int dx = floor(x) + 1;
	int dy = floor(y) + 1;

	//replace with a more dedicated noise function?
	short checkA = !caveNoise(ax,ay); 
	short checkB = !caveNoise(bx,by); 
	short checkC = !caveNoise(cx,cy);
	short checkD = !caveNoise(dx,dy);

	short cornerMask = checkA << 3 | checkB << 2 | checkC << 1 | checkD;

	switch (cornerMask) {
	case 0:
		//no terrain in this square
		return 0;
	case 1:
		//D only, top right only.
		// line goes from top to right.
		return (x - ax + y - ay > 1.5) * (cornerMask);
	case 2:
		// C only, top left only
		return (dx - x + y - ay > 1.5) * (cornerMask);
	case 3:
		//C and D: top left and top right;
		return (y - ay > 0.5) * (cornerMask);
	case 4:
		//bot right only?
		//	line goes from bot to right.
		return (x - ax + dy - y > 1.5) * (cornerMask);
	case 5:
		//0101
		// B and D: bot right, top right
		//
		return (x - ax > 0.5)*(cornerMask);
	case 9:
	case 6:
		//0110: B and C: bot right, top left)
		printf("error: messed up geometry");
		return -1;
	case 7:
		//0111: BCD, all but bot left
		return(x - ax + y - ay > 0.5)*(cornerMask);
	case 8:
		//1000: A only
		return (dx - x + dy - y > 1.5)*(cornerMask);
	case 10:
		//1010: A and C
		return (x - ax < 0.5)*(cornerMask);
	case 11:
		//1011 all but B
		return (dx - x + y - ay > 0.5)*(cornerMask);
	case 12:
		//1100 A and B
		return (y - ay < 0.5)*(cornerMask);
	case 13:
		//1101 all but C
		return (x - ax + dy - y > 0.5) * (cornerMask);
	case 14:
		//1110 all but D
		return (x - ax + y - ay < 1.5) * (cornerMask);
	case 15:
		//1111 add
		printf("error: messed up geometry (deep collision) \n");
		return -1;
	}	
	printf("error: marching squares no case?");
	return -1;
}

//it might be time to split into multiple files
//TODO: change pixel drawing func to not create a new rect every time? just change the . check if atually improves performance though.	

SDL_Texture* texFromBMP(const char* filepath, SDL_BlendMode bmode, Uint8 alphaMod) {
	SDL_Surface* tempSurface;
	tempSurface = SDL_LoadBMP(filepath);
	if (tempSurface == NULL) {
		printf("ERROR: couldn't load bmp from ");
		printf(filepath);
		printf("\n");
		return NULL;
	}
	SDL_ConvertSurfaceFormat(tempSurface, SDL_PIXELFORMAT_RGBA8888, 0); //maybe not necessary? who knows
	SDL_Texture* result = SDL_CreateTextureFromSurface(renderer, tempSurface);
	if (result == NULL) {
		printf("ERROR: Couldn't create texture from ");
		printf(filepath);
		printf("\n");
		return NULL;
	}
	SDL_SetTextureBlendMode(result, bmode);
	if (alphaMod != -1) {
		SDL_SetTextureAlphaMod(result, alphaMod);
	}
	SDL_FreeSurface(tempSurface);
	return result;
}

void loadImages() {
	//Load player Submarine sprite
	playerSubmarineTexture = texFromBMP("imageFiles/playerSubmarine.bmp", SDL_BLENDMODE_BLEND, -1);

	//load player flashlight sprite
	flashlightTexture = texFromBMP("imageFiles/flashlight.bmp", SDL_BLENDMODE_BLEND, -1);

	//load bubble sprite
	bubbleTexture = texFromBMP("imageFiles/bubble.bmp", SDL_BLENDMODE_MUL, 0);

	//load meters sprite
	metersTexture = texFromBMP("imageFiles/meters.bmp", SDL_BLENDMODE_BLEND, 255);

	//load starry sky sprite
	starrySkyTexture = texFromBMP("imageFiles/starrySky.bmp", SDL_BLENDMODE_NONE, 255);

	//load moon sprite
	moonTexture = texFromBMP("imageFiles/moon.bmp", SDL_BLENDMODE_NONE, 255);

	//load cloud texture
	cloudTexture = texFromBMP("imageFiles/clouds.bmp", SDL_BLENDMODE_BLEND, 200);
}

void initializeEverything() {
	printf("initializing everything :)");

	//first, we initialize SDL
	SDL_Init(SDL_INIT_VIDEO);

	printf("numAlloc: %d\n", SDL_GetNumAllocations());

	block.h = pixelSize;
	block.w = pixelSize;

	//create window
	window = SDL_CreateWindow("Cave Game Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);


	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == NULL) {
		printf("ERROR: RENDERER CREATION FAILED\n");
		printf(SDL_GetError());
	}
	printf("drawcolor succeeded?: %d", SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255));

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

	//now get window surface
	windowSurface = SDL_GetWindowSurface(window);
	if (windowSurface == NULL) {
		printf("ERROR: couldn't get window surface\n");
	}

	layerTwo = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_PIXELFORMAT_RGBA8888);

	SDL_SetSurfaceBlendMode(layerTwo, SDL_BLENDMODE_BLEND);

	if (window == NULL) {
		printf("ERROR: WINDOW CREATION FAILED\n");
	}

	//fill pixelDistance lookup table
	fillLookupTables();
	fillTraceBackTable();

	//initialize the terrain array:
	for (int i = 0; i < GAME_WIDTH; i++) {
		for (int j = 0; j < GAME_HEIGHT; j++) {
			caveTerrain[i][j] = caveNoise(i, j);
		}
	}

	//texture for light emitting from submarine
	lightingTexture = createLightingTexture(10, SDL_BLENDMODE_ADD, 100);

	darknessTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetTextureBlendMode(darknessTexture, SDL_BLENDMODE_BLEND);

	loadImages();

	finalBuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetTextureBlendMode(finalBuffer, SDL_BLENDMODE_NONE);
	SDL_SetRenderTarget(renderer, finalBuffer);

	int width = 0;
	int height = 0;
	SDL_QueryTexture(playerSubmarineTexture, NULL, NULL, &width, &height);
	submarineBuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, pixelSize*width, pixelSize*height);
	SDL_SetTextureBlendMode(submarineBuffer, SDL_BLENDMODE_BLEND);

	//not sure yet if i need this layer
	lightingBuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, GAME_WIDTH, GAME_HEIGHT);
	SDL_SetTextureBlendMode(lightingBuffer, SDL_BLENDMODE_BLEND); //try blend if this doesnt work? but i think it will.
	flashLightingTexture = createLightingTexture(100, SDL_BLENDMODE_NONE, 255);

	//SDL_ShowCursor(SDL_DISABLE);
}

void clearBuffers() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);

	SDL_SetRenderTarget(renderer, NULL);
	SDL_RenderClear(renderer);

	SDL_SetRenderTarget(renderer, finalBuffer);
	SDL_RenderClear(renderer);

	SDL_SetRenderTarget(renderer, submarineBuffer);
	SDL_RenderClear(renderer);

	SDL_SetRenderTarget(renderer, lightingBuffer);
	SDL_RenderClear(renderer);

	//special clear for darkness texture
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_SetRenderTarget(renderer, darknessTexture);
	SDL_RenderClear(renderer);
}

void composeSubmarine(SDL_RendererFlip charFlip, SDL_Rect* flashlightPos, float mouseAngle) {
	SDL_SetRenderTarget(renderer, submarineBuffer);
	SDL_RenderCopyEx(renderer, playerSubmarineTexture, NULL, NULL, 0, NULL, charFlip);
	SDL_RenderCopyEx(renderer, flashlightTexture, NULL, flashlightPos, mouseAngle, NULL, SDL_FLIP_NONE); //slap on the flashlight texture
}

void composeLighting(float mouseAngle) {
	SDL_SetRenderTarget(renderer, lightingBuffer);
	SDL_RenderCopy(renderer, flashLightingTexture, NULL, NULL);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);

	//set up vertices for cutting the flashlight into a triangle.
	float angleLeft = mouseAngle - 15;
	float angleRight = mouseAngle + 15;

	SDL_Vertex vertList[6];
	vertList[0] = { {(GAME_WIDTH / 2) + GAME_WIDTH * cosf(angleLeft * toRad)         ,  +1 + (GAME_HEIGHT / 2) + GAME_HEIGHT * sinf(angleLeft * toRad)}         ,  {0, 0, 0, 0}, {1, 1} };
	vertList[1] = { {(GAME_WIDTH / 2) - GAME_WIDTH * cosf(angleLeft * toRad)         ,  +1 + (GAME_HEIGHT / 2) - GAME_HEIGHT * sinf(angleLeft * toRad)}         ,  {0, 0, 0, 0}, {1, 1} };
	vertList[2] = { {(GAME_WIDTH / 2) + GAME_WIDTH * cosf((angleLeft - 90) * toRad)  ,  +1 + (GAME_HEIGHT / 2) + GAME_HEIGHT * sinf((angleLeft - 90) * toRad)}  ,  {0, 0, 0, 0}, {1, 1} };

	vertList[3] = { {(GAME_WIDTH / 2) + GAME_WIDTH * cosf(angleRight * toRad)         ,  +1 + (GAME_HEIGHT / 2) + GAME_HEIGHT * sinf(angleRight * toRad)}        ,  {0, 0, 0, 0}, {1, 1} };
	vertList[4] = { {(GAME_WIDTH / 2) - GAME_WIDTH * cosf(angleRight * toRad)         ,  +1 + (GAME_HEIGHT / 2) - GAME_HEIGHT * sinf(angleRight * toRad)}        ,  {0, 0, 0, 0}, {1, 1} };
	vertList[5] = { {(GAME_WIDTH / 2) + GAME_WIDTH * cosf((angleRight + 90) * toRad)  ,  +1 + (GAME_HEIGHT / 2) + GAME_HEIGHT * sinf((angleRight + 90) * toRad)} ,  {0, 0, 0, 0}, {1, 1} };

	SDL_RenderGeometry(renderer, NULL, vertList, 6, NULL, 0);
	//SDL_SetTextureAlphaMod(lightingTexture, 160);
	//SDL_RenderCopy(renderer, lightingTexture, NULL, NULL);
}

void renderBubbles(int bQFront, int bQEnd, bubble* bubbleQueue) {
	int ind = bQFront;
	SDL_Rect bubbleRect;

	SDL_SetRenderTarget(renderer, finalBuffer);

	while (ind != bQEnd) {
		bubbleRect.x = (bubbleQueue[ind].x + GAME_WIDTH / 2 - playerX) * pixelSize;
		bubbleRect.y = (bubbleQueue[ind].y + GAME_HEIGHT / 2 - playerY) * pixelSize;
		//printf("bubble disp location: %d,%d \n", bubbleRect.x, bubbleRect.y);
		bubbleRect.h = 15;
		bubbleRect.w = 15;
		if (bubbleQueue[ind].y > -840) {
			SDL_RenderCopy(renderer, bubbleTexture, NULL, &bubbleRect);
		}
		ind = (ind + 1) * (ind < 100); //increment ind or set to 0 if overflows
	}
}

void renderClouds(SDL_Rect viewportRect) {
	float camXPosWorld = playerX + viewportRect.x / pixelSize;
	float camYPosWorld = playerY + viewportRect.y / pixelSize;

	SDL_Rect cloudSourceRect = { 0,0, 2000,60}; //select whole cloud bar
	SDL_Rect cloudDestRect = { viewportRect.x, viewportRect.y, 2000 * pixelSize, 300};//printed from viewport rect
	//camera x pos in world!
	
	camXPosWorld += 1000;
	camYPosWorld += 860;
	cloudDestRect.x -= camXPosWorld;
	cloudDestRect.x = -ringMod(-cloudDestRect.x, 1800*pixelSize);
	
	cloudDestRect.y -= camYPosWorld;
	
	//SDL_Rect cloudDestRect = {-playerX - 1000 + viewportRect.x/pixelSize, (- 820 - playerY + viewportRect.y/pixelSize), pixelSize*2000, 300};
	SDL_RenderCopy(renderer, cloudTexture, &cloudSourceRect, &cloudDestRect);
}

void renderNightSky(SDL_Rect viewportRect) {
	//TODO: add a set renderTarget? to final buffer I think? just incase.
	//should always be drawn from The top of the screen to water surface.
	//height should be diff between playerY and water (at -790)
	SDL_RenderCopy(renderer, starrySkyTexture, NULL, &viewportRect);
	//if (playerY < -790) {
		renderClouds(viewportRect);
	//}
}

void renderTurbulentWater(SDL_Vertex gradientCorner, SDL_Rect *viewportRect) {
	//height above water level:
	SDL_SetRenderDrawColor(renderer, 50, 50, 50, 50);

	SDL_Rect waterRect = { 0,gradientCorner.position.y, pixelSize, 0 };
	for (int i = 0; i < SCREEN_WIDTH; i += pixelSize) {
		float waterHeight = 5*(1+SimplexNoise::noise((i+(playerX*pixelSize + viewportRect->x)) / 100.0, SDL_GetTicks() / 1200.0));
		waterRect.x = i;
		waterRect.y -= floor(waterHeight);
		waterRect.h = floor(waterHeight);
		SDL_RenderFillRect(renderer, &waterRect);
		waterRect.y = gradientCorner.position.y;
	}
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

//TODO: maybe remove blendmode arg if turns out not necessary
void renderSunGradient(SDL_Rect* viewportRect, SDL_BlendMode bmode) {

	SDL_Vertex sunGradientVertex[4];
	sunGradientVertex[0] = { {0, ceilf((- 790 - playerY)*pixelSize)}, {50,50,50,50}, {1,1}}; //top left of grad. might have to mult by something
	sunGradientVertex[1] = { {SCREEN_WIDTH, ceilf(( - 790 - playerY)*pixelSize)}, {50,50,50,50}, {1,1}}; //top right of grad
	sunGradientVertex[2] = { {0,  ceilf((-350 - playerY)*pixelSize)}, {0,0,0,0}, {1,1} }; //
	sunGradientVertex[3] = { {SCREEN_WIDTH, ceilf((-350 - playerY)*pixelSize)}, {0,0,0,0}, {1,1} };
	
	int sunGradInd[6] = { 0,1,2,1,2,3 };

	SDL_SetRenderDrawBlendMode(renderer, bmode);
	SDL_RenderGeometry(renderer, NULL, sunGradientVertex, 4, sunGradInd, 6);
	renderTurbulentWater(sunGradientVertex[0], viewportRect);

	/*
	unsigned char bright = (-550 - playerY) / 2;
	int excess = 0;
	if (playerY < -790) {
		excess = (-790 - playerY) * pixelSize;
		//printf("excess: %d", excess);
		//printf("playerY: %f", playerY);
		bright = (-550 + 790) / 2;
	}
	unsigned char dark = bright / 2;
	SDL_Vertex sunGradientVert[4];
	sunGradientVert[0] = { {0,          (float)excess}, {bright, bright, bright, bright}, {1,1} };
	sunGradientVert[1] = { {SCREEN_WIDTH, (float)excess}, {bright, bright, bright, bright}, {1,1} };
	sunGradientVert[2] = { {0,          SCREEN_HEIGHT  }, {dark,   dark,   dark,   bright}, {1,1} };
	sunGradientVert[3] = { {SCREEN_WIDTH, SCREEN_HEIGHT  }, {dark,   dark,   dark,   bright}, {1,1} };


	int sunGradInd[6] = { 0,1,2,1,2,3 };
	*/
	//SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
	//SDL_SetRenderTarget(renderer, lightingBuffer);

	//SDL_RenderGeometry(renderer, NULL, sunGradientVert, 4, sunGradInd, 6);

}

void renderRedFilter(int collisionCheck, float* healthRemaining, float* redSlider, float frameDelta) {
	if (collisionCheck > 0) {
		*healthRemaining -= 2;
		*redSlider = 150;
	}
	else {
		*redSlider -= 1 * frameDelta;
		if (*redSlider < 0) {
			*redSlider = 0;
		}
	}
	if (*redSlider > 0) {
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, (int)*redSlider);
		SDL_RenderFillRect(renderer, NULL);
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
	}
}

int main(int argc, char* args[]) {
	//gameplay variables
	const int airCapacity = 100;
	float airRemaining = 86;
	
	const int powerCapacity = 100;
	float powerRemaining = 43;

	const int healthCapacity = 100;
	float healthRemaining = 60;

	//variables for padding around screen for camera lag
	const int viewportPad = 40;
	const int vPadSpeedConstant = 200;
	
	//portion of speed remaining after collision 
	const float bumpFactor = 0.25;

	SDL_Rect subDestRect;
	subDestRect.x = (GAME_WIDTH/2 - 5)*pixelSize;
	subDestRect.y = (GAME_WIDTH/2 - 4)*pixelSize;
	subDestRect.w = 10 * pixelSize;
	subDestRect.h = 7 * pixelSize;
	
	SDL_Rect viewportRect;
	viewportRect.x = viewportPad;
	viewportRect.y = viewportPad;
	viewportRect.w = SCREEN_WIDTH - (2 * viewportPad);
	viewportRect.h = SCREEN_WIDTH - (2 * viewportPad);

	//this will all have to change later
	SDL_Rect flashlightPos;
	flashlightPos.x = 6*pixelSize;
	flashlightPos.y = 7*pixelSize;
	flashlightPos.w = 4*pixelSize;
	flashlightPos.h = 4*pixelSize;
		

	SDL_RendererFlip charFlip = SDL_FLIP_NONE;
	
	int frameCount = 0;
	
	initializeEverything();

	bool quit = false;

	SDL_Event event;
	
	//mouse position variables
	int xMouse = 0;
	int yMouse = 0;

	SDL_GetMouseState(&xMouse, &yMouse);

	float mouseAngle = atan2f(yMouse - (SCREEN_HEIGHT/2), xMouse - (SCREEN_WIDTH/2)) * 180 / 3.1415;

	//bools to keep track of where/if we are accelerating
	bool pushingLeft = false;
	bool pushingUp = false;
	bool pushingRight = false;
	bool pushingDown = false;
	bool explode = false;
	bool explodePrev = false;

	const int fpsWindowSize = 100;

	uint32_t prevTicks = -1000;
	uint32_t lastNFrames[fpsWindowSize] = {};
	
	std::chrono::steady_clock::time_point oldTime = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point newTime;
	std::chrono::steady_clock::duration frameTime;

	float redSlider = 0;

	//index of oldest bubble
	int bQFront = 0;
	//index of newest bubble
	int bQEnd = 0;
	bubble bubbleQueue[100] = {};

	//hashmap to store explosions:
	//std::unordered_map<int, std::unordered_map<int, bool>> explosions;

	bool tickChange = 1;
	Uint32 lastTickChange = SDL_GetTicks();
	//length of a tick in milliseconds
	const Uint32 tickLength = 50;

	int prevX = floor(playerX);
	int prevY = floor(playerY);

	//MAIN LOOP!!!
	while (!quit) {
		if (SDL_GetTicks() - lastTickChange > tickLength) {
			tickChange = 1;
			lastTickChange = SDL_GetTicks();
		}
		else {
			tickChange = 0;
		}
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
				case SDLK_w:
				case SDLK_UP:
					pushingDown = true;
					break;
				case SDLK_s:
				case SDLK_DOWN:
					pushingUp = true;
					break;
				case SDLK_a:
				case SDLK_LEFT:
					pushingLeft = true;
					break;
				case SDLK_d:
				case SDLK_RIGHT:
					pushingRight = true;
					break;
				case SDLK_q:
					explode = true;
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
				case SDLK_w:
				case SDLK_UP:
					pushingDown = false;
					yAccel = 0;
					break;
				case SDLK_s:
				case SDLK_DOWN:
					pushingUp = false;
					yAccel = 0;
					break;
				case SDLK_a:
				case SDLK_LEFT:
					pushingLeft = false;
					xAccel = 0;
					break;
				case SDLK_d:
				case SDLK_RIGHT:
					pushingRight = false;
					xAccel = 0;
					break;
				case SDLK_q:
					explode = false;
					break;
				default:
					break;
				}
			}
		}

		//now do an explosion (just for testing!!!)
		//maybe do a set for the second one instead of a map? figure out later. prob not cause should store size as well
		//OK I WANT TO MAKE IT SO THAT CHECKING THE THING IS SUPER EASY. SO I SHOULD MAKE THE EXPLOSIONS SMALL, BUT HAVE THIS PIECE OF CODE MAKE A LOT OF THEM. wILL TAKE MORE TIME DURING EXPLOSION, BUT THEN CHECKING AFTER WILL BE SUPER EASY.
		if (explode && !explodePrev) {
			printf("making explosion!!!1!!");
			// switch bool 
			for (float i = playerX + (GAME_WIDTH / 2) - expGrain*expSize - 0.5; i < playerX + (GAME_WIDTH / 2) + expGrain * expSize; i+= expGrain) {
				for (float j = playerY + (GAME_HEIGHT / 2) - expGrain * expSize- 0.5; j < playerY + (GAME_HEIGHT / 2) + expGrain * expSize; j += expGrain) {
					if ((abs(playerY +(GAME_HEIGHT / 2) - j) * abs(playerY + (GAME_HEIGHT / 2) - j)) + (abs(playerX + (GAME_WIDTH / 2) - i) * abs(playerX + (GAME_WIDTH / 2) - i)) < expGrain * expSize * expGrain * expSize + 0.5) {
						if (explosions.find(std::pair<int,int>(round(i / expGrain), round(j / expGrain))) == explosions.end()) {
							explosions[std::pair<int, int>(round(i / expGrain), round(j / expGrain))] = 1;
							printf("added new block!");
						}
					}
				}
			}
			/*
			if (explosions.find(round((playerX + GAME_WIDTH/2)/8)) == explosions.end()) {
				std::unordered_map<int, bool> newMap;
				explosions[round((playerX + GAME_WIDTH/2)/ 8)] = newMap;
				printf("added new map!!!");
			}
			else {
				printf("no new map!");
			}
			explosions[round((playerX + GAME_WIDTH/2) / 8)][round((playerY + GAME_HEIGHT/2)/8)] = true;
			*/
		}
		explodePrev = explode;

		//MAYBE IF WITH THE OTHER SO DONT RERELOAD.
		if (explode) {
			//refresh all the cave terrain?
			for (int i = (GAME_WIDTH / 2) - expGrain * expSize - 1; i < (GAME_WIDTH / 2) + expGrain * expSize+ 1; i++) {
				for (int j = (GAME_HEIGHT / 2) - expGrain * expSize - 1; j < (GAME_HEIGHT / 2) + expGrain * expSize + 1; j++) {
					caveTerrain[ringMod(i + bufferOffsetX, GAME_WIDTH)][ringMod(j + bufferOffsetY, GAME_HEIGHT)] = caveNoise(i + playerX, j + playerY);
				}
			}
		}


		//now adjust movement speed
		bool pushingSide = pushingLeft || pushingRight;
		bool pushingVert = pushingUp || pushingDown;

		if (pushingLeft) {
			if (pushingVert) {
				xAccel = -ACCEL_RATE / sqrt(2);
			}
			else {
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
			}
			else {
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
		int avgFPS = fpsWindowSize/(lastNFrames[frameCount % fpsWindowSize] - lastNFrames[(frameCount + 1)%fpsWindowSize]);
		printf("FPS: % d\n", (fpsWindowSize*1000)/(lastNFrames[frameCount % fpsWindowSize] - lastNFrames[(frameCount + 1) % fpsWindowSize]));
		printf("actualHMSize: %d\n", explosions.size());

		newTime = std::chrono::steady_clock::now();
		frameTime = std::chrono::duration_cast<std::chrono::microseconds>(newTime - oldTime);
		oldTime = std::chrono::steady_clock::now();
		//printf("frameMicros: %d\n", frameTime.count());

		float frameDelta = frameTime.count() / 2'000'000.0;

		float portion = 0;
		float waterHeight = (1 + SimplexNoise::noise((SCREEN_WIDTH/2 + (playerX * pixelSize + viewportRect.x)) / 100.0, SDL_GetTicks() / 1200.0))/1.5 - 1;

		if (playerY < -839 - waterHeight) {
			//in the air
			if (ySpeed < -0.3) {
				ySpeed = -0.3;
			}
			//portion !!underwater!!: how far below 844 we are
			portion = (844 + waterHeight + playerY) / 6;
			if (portion < 0) {
				portion = 0;
			}
			yAccel = 0.001 - portion / 500;
			//frameDelta This
			if (playerY > -844 - waterHeight) {
				ySpeed *= 1 - 0.001;
				xSpeed *= 1 - 0.001;
				if (pushingUp) {
					yAccel = ACCEL_RATE;
				}
			}
			else {
				xAccel = 0;
			}

		}
		else if (prevY < -841 - waterHeight) {
			yAccel = 0;
			xAccel = 0;
		}

		xSpeed += xAccel * frameDelta;
		ySpeed += yAccel * frameDelta;

		//TODO: figure out a way to frameDelta this
		if (prevY > -841 - waterHeight  && xAccel == 0) {
			xSpeed *= 1 - frameDelta / 200;
		}
		if (prevY > -841 - waterHeight && yAccel == 0) {
			ySpeed *= 1 - frameDelta / 200;
		}

		prevX = floor(playerX);
		prevY = floor(playerY);

		playerX += xSpeed * frameDelta;
		playerY += ySpeed * frameDelta;

		int collisionCheck = 0;
		float temp = xSpeed;

		//perform collision check on 4 corners
		for (int i = -2; i <= 2; i += 4) {
			for (int j = -2; j <= 2; j += 4) {
				collisionCheck = marchingSlope(playerX + i + GAME_WIDTH / 2, playerY + j + GAME_WIDTH / 2);
				if (collisionCheck > 0) {
					goto collided;
				}
			}
		}
		goto notCollided;
	collided:
		if (collisionCheck == -1) { printf("error! deep collision"); quit = 1; }
		//abcd = bot left, bot right, top left, top right (in imaginary world)
		//ok there's been a collision
		//printf("%d ", collisionCheck);
		//undo previous movement. Now we are guaranteed (?) to be in open space
		playerX -= xSpeed * frameDelta;
		playerY -= ySpeed * frameDelta;

		switch (collisionCheck) {
		case 1:
		case 7:
		case 8:
		case 14:
			xSpeed = -ySpeed * bumpFactor;
			ySpeed = -temp * bumpFactor;
			break;
		case 13:
		case 11:
		case 4:
		case 2:
			xSpeed = ySpeed * bumpFactor;
			ySpeed = temp * bumpFactor;
			break;
		case 12:
		case 3:
			xSpeed = xSpeed * 2 * bumpFactor;
			ySpeed = -ySpeed * bumpFactor;
			break;
		case 10:
		case 5:
			xSpeed = -xSpeed * bumpFactor;
			ySpeed = ySpeed * 2 * bumpFactor;
			break;
		case 9:
		case 6:
		case 15:
			break;
		}

	notCollided:

		bufferOffsetX = ringMod(floor(playerX), GAME_WIDTH);
		bufferOffsetY = ringMod(floor(playerY), GAME_HEIGHT);

		//maybe we have to do it virtually.
		//ok dont use bufferoffset, just use discrete x and y

		//calculate new bufferOffset from player position
		bufferOffsetX = ringMod(floor(playerX), GAME_WIDTH);
		bufferOffsetY = ringMod(floor(playerY), GAME_HEIGHT);

		//ok now need buffer fill direction.
		int bufferDirectionX = (playerX - prevX >= 0) - (playerX - prevX < 0);
		int bufferDirectionY = (playerY - prevY >= 0) - (playerY - prevY < 0);

		// old way of determining this, that worked for everything except surprise collisions, which changed speed.
		//int bufferDirectionX = (xSpeed >= 0) - (xSpeed < 0);
		//int bufferDirectionY = (ySpeed >= 0) - (ySpeed < 0);

		int numNewLinesX = abs(floor(playerX) - prevX);
		int numNewLinesY = abs(floor(playerY) - prevY);

		//this code fills in parts of the buffer that are now off the screen with new noise
		int istart = 0;
		int iend = 0;

		if (bufferDirectionX == -1) {
			iend = numNewLinesX;
		}
		else {
			istart = GAME_WIDTH - numNewLinesX;
			iend = GAME_WIDTH;
		}
		for (int i = istart; i < iend; i++) {
			for (int j = 0; j < GAME_HEIGHT; j++) {
				caveTerrain[ringMod(i + bufferOffsetX, GAME_WIDTH)][ringMod(j + bufferOffsetY, GAME_HEIGHT)] = caveNoise(i + playerX, j + playerY);
			}
		}

		int jstart = 0;
		int jend = 0;

		if (bufferDirectionY == -1) {
			jend = numNewLinesY;
		}
		else {
			jstart = GAME_HEIGHT - numNewLinesY;
			jend = GAME_HEIGHT;
		}
		for (int i = 0; i < GAME_WIDTH; i++) {
			for (int j = jstart; j < jend; j++) {
				caveTerrain[ringMod(i + bufferOffsetX, GAME_WIDTH)][ringMod(j + bufferOffsetY, GAME_HEIGHT)] = caveNoise(i + playerX, j + playerY);
			}
		}

		//calculate subpixel offset for frame drawing purposes
		float subPixelDiffX = playerX - floor(playerX);
		float subPixelDiffY = playerY - floor(playerY);

		subPixelOffsetX = round(subPixelDiffX * (pixelSize - 1));
		subPixelOffsetY = round(subPixelDiffY * (pixelSize - 1));


		//bubble processing

		//pop oldest bubble if necessary
		if (bQFront != bQEnd && SDL_GetTicks() > bubbleQueue[bQFront].popTime) {
			printf("popped the bubble\n");
			bQFront++;
			if (bQFront == 100) {
				bQFront = 0;
			}
		}
		int ind = bQFront;
		//update bubble positions
		while (ind != bQEnd) {
			bubbleQueue[ind].x += bubbleQueue[ind].xSpd * frameDelta;
			bubbleQueue[ind].y += bubbleQueue[ind].ySpd * frameDelta;
			bubbleQueue[ind].xSpd *= 1 - frameDelta / 200;
			bubbleQueue[ind].ySpd *= 1 - frameDelta / 200;
			bubbleQueue[ind].ySpd -= .0001 * frameDelta;
			ind++;
			if (ind == 100) {
				ind = 0;
			}
		}
		//printf("made it past the while loop\n");

		if (tickChange && (pushingSide || pushingVert)) {
			//printf("making new bubble!\n");
			bubble newBub;
			newBub.popTime = SDL_GetTicks() + 4000;
			newBub.x = playerX + 4 * (2 * (xSpeed < 0) - 1) + (rand() % 10 - 5) / 3.0;
			newBub.y = playerY + (rand() % 10 - 5) / 2;
			newBub.xSpd = xSpeed / 2 /*+ (rand() % 10 - 5) / 10.0*/;
			newBub.ySpd = ySpeed / 2 /*+ (rand() % 10 - 5) / 10.0*/;
			bubbleQueue[bQEnd] = newBub;
			bQEnd++;
			if (bQEnd == 100) {
				bQEnd = 0;
			}
		}



		//calculate viewport lag
		//first draft: base it on speed. TODO: replace this stuff with constants (the 20s, the 100)
		viewportRect.x = viewportPad - xSpeed * vPadSpeedConstant;
		viewportRect.y = viewportPad - ySpeed * vPadSpeedConstant;
		if (viewportRect.x < 0) {
			viewportRect.x = 0;
		}
		if (viewportRect.y < 0) {
			viewportRect.y = 0;
		}
		if (viewportRect.x > 2 * viewportPad - 1) {
			viewportRect.x = 2 * viewportPad - 1;
		}
		if (viewportRect.y > 2 * viewportPad - 1) {
			viewportRect.y = 2 * viewportPad - 1;
		}

		SDL_GetMouseState(&xMouse, &yMouse);

		mouseAngle = atan2f(yMouse - (SCREEN_HEIGHT / 2), xMouse - (SCREEN_WIDTH / 2)) * 180 / 3.1415;

		if (xSpeed > 0) {
			charFlip = SDL_FLIP_NONE;
		}
		if (xSpeed < 0) {
			charFlip = SDL_FLIP_HORIZONTAL;
		}
		//start RENDERING THE FRAME

			//breakdown of rendering process
				//take black texture, poke transparent holes in it. thats darkness texture
				//take flashlighting texture (large bright thing, same size as screen). copy to lighting buffer.
					//draw transparency with rendergeometry so only the triangle gets through.
					// 
					//layer on the smaller lighting texture using blendmode ADD.
					// splat lightingbuffer onto final buffer.
					//compose the submarine texture. main sub body is partially transparent. Then BLEND onto finalBuffer texture so its partially lit.
						//IDEA, DO THIS OTHER WAY. LIGHTING TEXTURE ADDS ON TO SUB? THEN CAN DO SKY FIRST, THEN SUB, THEN LIGHTING AS AN ADD. THAT WOULD WORK!!
					//splat on the bubbles with Blendmode MUL, so they get lit up by the lighting texture.
					//splat darkness texture using blend (so black parts replace, trans parts do nothing.
							//OOOOORRRRRR
					//dont splat the darkness, draw a semitransparent gradient over the screen

	//ok so try the following:
		//first, compose sky. paste it onto finalbuffer (shifted, so that camera move will place it back in same place as always)
			//make sure to only print onto areas of the screen that will be sky at the end. that way, it wont show up under the water through the lighting texture.
		//next, compose the submarine (and make it opaque). And paste it on the final buffer.
		//next, compose lighting, and paste it onto the final scene as an ADD.
		// multiply on the bubbles.
		//next, splat the sunshine gradient on the scene. with a blend?
		//next, splat the viewport from final buffer onto the screen!!

		//clear out all buffers
		clearBuffers();

		//compose submarine
		composeSubmarine(charFlip, &flashlightPos, mouseAngle);

		//compose lighting
		composeLighting(mouseAngle);

		//compose darkness texture
		if (playerY > -550) {
			rowByRowLighting(GAME_WIDTH / 2, GAME_HEIGHT / 2, mouseAngle);
		}
		//ok now everything seems to be composed.
		//lets try layering




		//***BEGIN RENDERING***
		SDL_SetRenderTarget(renderer, finalBuffer);
		
		//render sky and clouds to finalBuffer.
		if (playerY < -550) {
			renderNightSky(viewportRect);
			//render opaque, black version of submarine (so you dont see the stars through it later)
			if (SDL_SetTextureColorMod(submarineBuffer, 0, 0, 0) == -1) {
				printf("ERROR: couldn't set color mod for submarine texture");
			}
			SDL_RenderCopy(renderer, submarineBuffer, NULL, &subDestRect);
			
			if (SDL_SetTextureColorMod(submarineBuffer, 255, 255, 255) == -1) {
				printf("ERROR: couldn't return color mod for submarine texture");
			}
		}
		if (playerY < -350) {
			//render water gradient and waves, no blending
			renderSunGradient(&viewportRect, SDL_BLENDMODE_NONE);
		}
		
		//render lighting
		//TODO: maybe redo alpha system in createLightingTexture() to work better with this new system
		SDL_SetTextureBlendMode(lightingBuffer, SDL_BLENDMODE_ADD);
		SDL_RenderCopy(renderer, lightingBuffer, NULL, NULL);
		SDL_RenderCopy(renderer, lightingTexture, NULL, NULL);

		SDL_SetTextureAlphaMod(submarineBuffer, 120);
		SDL_RenderCopy(renderer, submarineBuffer, NULL, &subDestRect);
		SDL_SetTextureAlphaMod(submarineBuffer, 255);

		//render bubbles onto texture
		renderBubbles(bQFront, bQEnd, bubbleQueue);

		//render darknessTexture
		if (playerY > -550) {
			SDL_RenderCopy(renderer, darknessTexture, NULL, NULL);
		}

		//render lightGradient
		//top of gradient should be at -841?
		//bottom should be at -550?
		

		renderRedFilter(collisionCheck, &healthRemaining, &redSlider, frameDelta);


		//render viewport to screen 
		SDL_SetRenderTarget(renderer, NULL);
		SDL_RenderCopy(renderer, finalBuffer, &viewportRect, NULL);
		
		//DEBUG
		//SDL_RenderClear(renderer);
		//SDL_RenderCopy(renderer, lightingBuffer, NULL, NULL);

		//finally update the screen
		SDL_RenderPresent(renderer);
		


		frameCount++;
		continue;



	

		//Poke holes in the darkness texture
		if (playerY > -550) {
			rowByRowLighting(GAME_WIDTH / 2, GAME_HEIGHT / 2, mouseAngle);
		}
		else {
			//Render night sky to finalbuffer
			SDL_Rect nightSkyTestRect = { 0,0,SCREEN_WIDTH, SCREEN_HEIGHT / 2 };
			SDL_SetRenderTarget(renderer, finalBuffer);
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, starrySkyTexture, NULL, &nightSkyTestRect);
			//printf("no more lighting");
		}

		//Render submarine to finalBuffer
		//Now let's compose the submarine texture
		
		if (xSpeed > 0) {
		charFlip = SDL_FLIP_NONE;
	}
	if (xSpeed < 0) {
		charFlip = SDL_FLIP_HORIZONTAL;
	}
		//now the submarine texture is complete, so we can put it on the finalbuffer
		SDL_SetRenderTarget(renderer, finalBuffer);
		SDL_RenderCopy(renderer, submarineBuffer, NULL, &subDestRect);


		//Compose lighting layer
		
		
		//render lightingbuffer to final buffer
		SDL_SetRenderTarget(renderer, finalBuffer);
		SDL_RenderCopy(renderer, lightingBuffer, NULL, NULL);

		//render bubbles to final buffer
		//have to add GW/2
		

		//Add darkness texture to finalBuffer
		if (playerY > -550) {
			SDL_RenderCopy(renderer, darknessTexture, NULL, NULL);
		}
		else {
			if (0 == 1) {
				//IDEA: just make this a giant rectangle that reaches up to -840. there will be then no weird behavior at the surface.
				//playerY < -840 is the surface
				unsigned char bright = (-550 - playerY) / 2;
				int excess = 0;
				if (playerY < -790) {
					excess = (-790 - playerY) * pixelSize;
					//printf("excess: %d", excess);
					//printf("playerY: %f", playerY);
					bright = (-550 + 790) / 2;
				}
				unsigned char dark = bright / 2;
				SDL_Vertex sunGradientVert[4];
				sunGradientVert[0] = { {0,          (float)excess}, {bright, bright, bright, bright}, {1,1} };
				sunGradientVert[1] = { {SCREEN_WIDTH, (float)excess}, {bright, bright, bright, bright}, {1,1} };
				sunGradientVert[2] = { {0,          SCREEN_HEIGHT  }, {dark,   dark,   dark,   bright}, {1,1} };
				sunGradientVert[3] = { {SCREEN_WIDTH, SCREEN_HEIGHT  }, {dark,   dark,   dark,   bright}, {1,1} };


				int sunGradInd[6] = { 0,1,2,1,2,3 };
				SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
				//SDL_SetRenderTarget(renderer, lightingBuffer);

				SDL_RenderGeometry(renderer, NULL, sunGradientVert, 4, sunGradInd, 6);
				//SDL_SetRenderTarget(renderer, finalBuffer);
				//SDL_RenderCopy(renderer, lightingBuffer, NULL, NULL);


				//render the nightSky
				//SDL_SetRenderTarget(renderer, NULL);
				//render stars texture first
				//SDL_Rect skyRect = { 0, (float)excess - SCREEN_HEIGHT,SCREEN_WIDTH, SCREEN_HEIGHT };
				//SDL_RenderCopy(renderer, starrySkyTexture, NULL, &skyRect);

				//render moon texture
				//skyRect = { 50,50,50,50 };
				//SDL_RenderCopy(renderer, moonTexture, NULL, &skyRect);

				//render clouds
				//skyRect = { 0,55, SCREEN_WIDTH, 60 };
				//SDL_Rect cloudRect = { ringMod(playerX,2000), 0, SCREEN_WIDTH, 60 };
				//SDL_RenderCopy(renderer, cloudTexture, &cloudRect, &skyRect);

				//SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
			}
		}

		//Now draw a portion of the window to the real place
		SDL_SetRenderTarget(renderer, NULL);
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, finalBuffer, &viewportRect, NULL);

		//Now draw extra effects like red danger filter:
		if (collisionCheck > 0) {
			healthRemaining -= 2;
			redSlider = 150;
		}
		else {
			redSlider -= 1*frameDelta;
			if (redSlider < 0) {
				redSlider = 0;
			}
		}
		if (redSlider > 0) {
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(renderer, 255, 0, 0, (int) redSlider);
			SDL_RenderFillRect(renderer, NULL);
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
		}

		//render menu items and meters to the screen
		SDL_Rect meterScreenRect = { 50, 50, 3*86, 3*7 };
		
		//select and draw empty meters
		SDL_Rect meterSelectRect = { 0, 7, 86, 7 };
		for (int i = 0; i < 3; i++) {
			SDL_RenderCopy(renderer, metersTexture, &meterSelectRect, &meterScreenRect);
			meterScreenRect.y += 30;
		}

		//select and draw air meter
		meterSelectRect.y = 14;
		meterSelectRect.w = (airRemaining / airCapacity) * 86;
		
		meterScreenRect.y = 50;
		meterScreenRect.w = 3*meterSelectRect.w;
		
		SDL_RenderCopy(renderer, metersTexture, &meterSelectRect, &meterScreenRect);

		//select and draw power meter
		meterSelectRect.y = 21;
		meterSelectRect.w = (powerRemaining / powerCapacity) * 86;

		meterScreenRect.y += 30;
		meterScreenRect.w = 3 * meterSelectRect.w;

		SDL_RenderCopy(renderer, metersTexture, &meterSelectRect, &meterScreenRect);

		//select and draw health meter;
		meterSelectRect.y = 28;
		meterSelectRect.w = (healthRemaining / healthCapacity) * 86;

		meterScreenRect.y += 30;
		meterScreenRect.w = 3 * meterSelectRect.w;

		SDL_RenderCopy(renderer, metersTexture, &meterSelectRect, &meterScreenRect);

		SDL_RenderPresent(renderer);

		//printf("frame done");
		frameCount++;
		//printf("OVR AVG FPS: %d", 1000 * frameCount / SDL_GetTicks());
		//quit = 1;

		airRemaining -= frameDelta/1000;
		if (playerY < -841) {
			airRemaining += frameDelta / 100;
			if (airRemaining > airCapacity) {
				airRemaining = airCapacity;
			}
		}
		powerRemaining -= frameDelta / 3000;
		powerRemaining -= (frameDelta / 500) * (pushingVert || pushingSide);

		//debug
		//printf("caveWidth factor: %f\n", getCaveWidth(playerY, playerX));
}

	//free up everything and quit
	//Destroy window
	SDL_DestroyWindow(window);

	SDL_FreeSurface(layerTwo);
	
	SDL_DestroyTexture(lightingTexture);

	SDL_DestroyTexture(flashLightingTexture);

	SDL_DestroyTexture(lightingBuffer); //again, not sure if this is really necessary

	SDL_DestroyTexture(darknessTexture);

	SDL_DestroyTexture(finalBuffer);

	SDL_DestroyTexture(playerSubmarineTexture);

	SDL_DestroyTexture(flashlightTexture);

	SDL_DestroyTexture(submarineBuffer);

	SDL_DestroyTexture(bubbleTexture);

	SDL_DestroyTexture(metersTexture);

	SDL_DestroyTexture(starrySkyTexture);

	SDL_DestroyTexture(moonTexture);

	SDL_DestroyTexture(cloudTexture);

	SDL_DestroyRenderer(renderer);

	printf("numAlloc: %d\n", SDL_GetNumAllocations());
	//Quit SDL subsystems
	SDL_Quit();

	printf("numAlloc: %d\n", SDL_GetNumAllocations());
	return 0;
}


