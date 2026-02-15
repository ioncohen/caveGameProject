#pragma once
#include <vector>
#include <unordered_set>

//This class will be a sort of oracle that can tell you the worldstate at global positions.
//It will maintain an array for the state of the area of the world shown on screen. As the player moves, it will overwrite information about the edges of the screen and replace it with the new area.
//It will also be able to answer queries about arbitrary world positions, not sure if we are going to need that but just in case.
//As an interface, you will be able to query for global indices, or for different offsets from the player.
//Eventually there will be a resize function which will cause the whole current array to be discarded and replaced with a larger array. This will be necessary for changing resolutions maybe?

struct PairHash {
	std::size_t operator()(const std::pair<int, int>& p) const noexcept {
		std::size_t h1 = std::hash<int>{}(p.first);
		std::size_t h2 = std::hash<int>{}(p.second);
		return h1 ^ (h2 << 1);
	}
};

class GameWorldOracle {
public:
	GameWorldOracle(int startingPositionX, int startingPositionY, const int GAME_X, const int GAME_Y);

	bool queryAbsoluteWorldState(int queryX, int queryY);
	bool queryOffsetWorldState(int xFromCenter, int yFromCenter);
	
	void addExplosion(int explosionX, int explosionY, short explosionRadius);
	void addExplosion(int explosionX, int explosionY);

	void updateWorldArray(int playerX, int playerY);

private:
	//General variables for world map
	std::vector<char> caveTerrain;
	int width, height;
	int xOffset = 0;
	int yOffset = 0;

	//Storage for explosions
	std::unordered_set<std::pair<int, int>, PairHash> explosionMap;

	char& at(int x, int y);
};