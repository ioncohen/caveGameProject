#include "GameWorldOracle.h"
#include "SimplexNoise.h"

static constexpr float noiseCutoffLevel = 0;
static constexpr float noiseScaleFactor = 50;
static constexpr int openOceanHeight = 344;
static constexpr int surfaceHeight = 844;

static bool caveNoise(int x, int y);
static float getCaveWidth(float x, float y);
static int ringMod(int a, int b);

GameWorldOracle::GameWorldOracle(int startingPositionX, int startingPositionY, const int GAME_X, const int GAME_Y) {
	width = GAME_X;
	height = GAME_Y;
	caveTerrain = std::vector<char>(width * height);

	//xOffset and yOffset should essentially be thought of as what is the location in the world the BOTTOM LEFT CORNER of the array is in.
	xOffset = startingPositionX - width / 2;
	yOffset = startingPositionY - height / 2;


	//fill the initial world array.
	for (int i = xOffset; i < xOffset + width; i++) {
		for (int j = yOffset; j < yOffset + height; j++) {
			//i,j are real coordinates. the at() function converts to the corresponding internal coordinates.
			at(i, j) = caveNoise(i, j);
		}
	}
}

//Takes in real world x and ys, converts to modded coordinates, then converts to flat vector coords
char& GameWorldOracle::at(int x, int y) {
	int moddedX = ringMod(x, width);
	int moddedY = ringMod(y, height);

	return caveTerrain[moddedX + moddedY * width];
};

//Implicitly casts from char& to bool; hope this works.
bool GameWorldOracle::queryAbsoluteWorldState(int queryX, int queryY) {
	return at(queryX, queryY);
}

//May be used to simplify some logic for callers. Might remove this
bool GameWorldOracle::queryOffsetWorldState(int xFromCenter, int yFromCenter) {
	return at(xFromCenter + xOffset + width / 2, yFromCenter + yOffset + height / 2);
}

void GameWorldOracle::addExplosion(int x, int y) {
	explosionMap.emplace(x, y);
}

void GameWorldOracle::addExplosion(int x, int y, short explosionRadius) {
	for (int i = -explosionRadius; i < explosionRadius; i++) {
		for (int j = -explosionRadius; j < explosionRadius; j++) {
			if (i * i + j * j <= explosionRadius * explosionRadius) {
				explosionMap.emplace(x + i, y + i);
			}
		}
	}
}

void GameWorldOracle::updateWorldArray(int playerX, int playerY) {
	//We have the current xOffset and yOffset. playerX and playerY are the new ones. So we can deduce which new points need to be iterated through?
	int xChange = playerX - (xOffset + width / 2);
	int yChange = playerY - (yOffset + height / 2);

	//Loop through the new points and overwrite in the buffer
	
	if (xChange < 0) {
		//Went left. Need to add leftmost lines to the buffer
		for (int i = playerX - width / 2; i < xOffset; i++) {
			for (int j = playerY - height / 2; j < playerY + height / 2; j++) {
				at(i, j) = caveNoise(i, j);
			}
		}
	}
	else if (xChange > 0) {
		//Went right. Need to add rightmost lines to the buffer
		for (int i = playerX + width / 2; i > xOffset + width; i--) {
			for (int j = playerY - height / 2; j < playerY + height / 2; j++) {
				at(i, j) = caveNoise(i, j);
			}
		}
	}
	if (yChange < 0) {
		//Went down. Need to add bottommost lines to the buffer
		for (int i = playerX - width / 2; i < playerX + width / 2; i++) {
			for (int j = playerY - height / 2; j < yOffset; j++) {
				at(i, j) = caveNoise(i, j);
			}
		}
	}
	else if (yChange > 0) {
		//Went up. Need to add uppermost lines to the buffer
		for (int i = playerX - width / 2; i < playerX + width / 2; i++) {
			for (int j = playerY + height / 2; j > yOffset + height; j--) {
				at(i, j) = caveNoise(i, j);
			}
		}
	}
}

//mod method that loops around for negative numbers
static int ringMod(int a, int b) {
	int ret = a % b;
	return (ret < 0) ? ret + b : ret;
}

//This should probably be within a different WorldGeneration or WorldGenerator class. But for now, for testing, this is ok.
static bool caveNoise(int x, int y) {
	//generate the width modifier from y value.
	//return the noise check
	return SimplexNoise::noise(x / noiseScaleFactor, y / noiseScaleFactor) + getCaveWidth(x, y) > noiseCutoffLevel;
}

static float getCaveWidth(float x, float y) {
	if (y < -400 + 20 * SimplexNoise::noise(x / 50.0)) { return 1; }
	return SimplexNoise::noise(x / (100 * noiseScaleFactor), y / (100 * noiseScaleFactor)) - 0.1;
}