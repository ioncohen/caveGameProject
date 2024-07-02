#include <SDL.h>
#include <stdio.h>
#include "SimplexNoise.h"
#include <ctime>
#include <cstdlib>

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

SDL_Color textColor = { 255, 255, 255, 255 };

const int SCREEN_WIDTH = 1010;
const int SCREEN_HEIGHT = 1010;

const int GAME_WIDTH = 333;
const int GAME_HEIGHT = 333;

const int MAX_DIM = GAME_WIDTH - (GAME_WIDTH - GAME_HEIGHT) * (GAME_HEIGHT > GAME_WIDTH);

const float ACCEL_RATE = 0.01;

//The window we'll be rendering to
SDL_Window* window = NULL;

//The surface contained by the window
SDL_Surface* windowSurface = NULL;

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
int pixelSize = 3;
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

void setBigPixel(int x, int y, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
	
	//outer pixels (add a +1 for a thing)
	x = (pixelSize) * x - subPixelOffsetX;
	y = (pixelSize) * y - subPixelOffsetY;


	for (int i = 0; i < pixelSize; i++) {
		for (int j = 0; j < pixelSize; j++) {
			if ((i < pixThick || i >= pixelSize - pixThick) || (j < pixThick || j >= pixelSize - pixThick)) {
				set_pixel(windowSurface, x + i, y + j, red, green, blue, alpha);
			}
			else{
				set_pixel(windowSurface, x + i, y + j, 2*red / 3, 2*green / 3, 2*blue / 3, alpha);
			}
		}
	}
	

	//for (int i = 0; i < pixelSize-1; i++) {
	//	set_pixel(windowSurface, x + i, y, red, green, blue, alpha);
	//	set_pixel(windowSurface, x + pixelSize - 1, y + i, red, green, blue, alpha);
	//	set_pixel(windowSurface, x + i + 1, y + pixelSize - 1, red, green, blue, alpha);
	//	set_pixel(windowSurface, x, y + i + 1, red, green, blue, alpha);
	//}
}	
//might not be necessary, can we get pixels from the screen instead?
//int lightingArray[][];

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

float rises[25] = { 0, 1, 1, 1, 2, 4, 1,  4,  2,  1,  1,  1,  0, -1, -1, -1, -2, -4, -1, -4, -2, -1 ,-1 ,-1, 0 };
float runs[25] = { 1, 4, 2, 1, 1, 1, 0, -1, -1, -1, -2, -4, -1, -4, -2, -1, -1, -1,  0, 1,   1,  1,  2,  4, 1 };
float snapAngles[25];

//TODO: test if theres overshooting for pixels near the center. might have to edit those. not a big deal though i think.
void fillTraceBackTable() {
	printf("STARTING FILLTRACEBACKTABLE");
	for (int i = 0; i < 25; i++) {
		snapAngles[i] = atan2f(rises[i], runs[i]) * 180 / 3.14159;
		if (snapAngles[i] < 0 && snapAngles[i] > -2) { snapAngles[i] = 0; }
		if (snapAngles[i] < 0) { snapAngles[i] += 360; }
	}
	snapAngles[24] = 360;
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
			
				for (int k = 0; k < 24; k++) {
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
		setBigPixel(x, y, 0, 0, 0); //is this necessary?
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


int main(int argc, char* args[]) {
	
	int frameCount = 0;

	//first, we initialize SDL
	SDL_Init(SDL_INIT_VIDEO);
	
	srand(time(0));

	//create window
	window = SDL_CreateWindow("Cave Game Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	
	//now get window surface
	windowSurface = SDL_GetWindowSurface(window);

	//ok now we can start our main loop:
	bool quit = false;
	SDL_Event event;

	//fill pixelDistance lookup table
	fillLookupTables();
	fillTraceBackTable();

	//bools to keep track of where/if we are accelerating
	bool pushingLeft = false;
	bool pushingUp = false;
	bool pushingRight = false;
	bool pushingDown = false;

	//initialize the terrain array:
	for (int i = 0; i < GAME_WIDTH; i++) {
		for (int j = 0; j < GAME_HEIGHT; j++) {
			caveTerrain[i][j] = SimplexNoise::noise(i / noiseScaleFactor, j / noiseScaleFactor) > noiseCutoffLevel;
		}
	}

	int prevBufferOffsetX = 0;
	int prevBufferOffsetY = 0;

	bool debugStepFlag = 0;

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
						debugStepFlag = true;
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

		/*if (!debugStepFlag) {
			continue;
		}
		*/

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
		xSpeed += xAccel;
		ySpeed += yAccel;

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

		playerX += xSpeed;
		playerY += ySpeed;

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
		
		subPixelOffsetX = round(subPixelDiffX * 2);
		subPixelOffsetY = round(subPixelDiffY * 2);

		//at this point our cave terrain array shoul dbe good. it has all the right data storred in the weird way in all the right places. So we just have to print it out using the buffer?
		handleLighting(GAME_WIDTH/2, GAME_HEIGHT/2);
		
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

	//Quit SDL subsystems
	SDL_Quit();

	return 0;
}

void draw50Lines(){
	for (int i = 0; i < 40; i++) {
		int startX = rand() % 333;
		int startY = rand() % 333;

		int endyX = GAME_WIDTH / 2;
		int endyY = GAME_HEIGHT / 2;

		while (startX != endyX || endyY != startY) {
			setBigPixel(startX, startY, 255, 255, 255);
			startX += pixTraceBackDirX[startX - endyX + GAME_WIDTH][startY - endyY + GAME_HEIGHT];
			startY += pixTraceBackDirY[startX - endyX + GAME_WIDTH][startY - endyY + GAME_HEIGHT];
		}
	}
}


//heres the plan:
//	in main loop, we check for button pushes, adjust speed of movement
//	apply movement (rotation and position)
//		do that by adjusting modulo of buffer
//	fill in new rows with shit.

//	now display:
//		spiral out
//		each pixel borrows from the ones on the way back home, if they're in the cone.
//			if you are a wall and you borrow from non wall, brighten yourself
//			if you are a wall and you borrow from a wall, set self to 0
// fill whole screen like that.
//		Maybe in that process of coloring, we can decide whether to use the tileable thing or the full thing.
// maybe loop through once and fill in the cores, and then loop through again and fill in the highlights
// 