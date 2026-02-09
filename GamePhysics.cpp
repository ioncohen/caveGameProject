#include "GamePhysics.h"
#include <iostream>

GamePhysics::GamePhysics() {};

//I guess i should probably make this more generic. updateObjectPosition. Then can just have a function that calculates different forces in different ways for each type of object?
void GamePhysics::updatePlayerPosition(PlayerCharacter& player, const InputState& inputs, int gameWorld, float deltaTime) {
	std::cout << "INITIAL_STATE" << std::endl;
	std::cout << "dt: " << deltaTime << std::endl;
	player.getPhysicsBody().printState();
	
	//Takes in player state (vehicle state, charge, etc) and inputs, and fills in xAccel and yAccel.
	calculatePlayerForces(player, inputs, deltaTime);
	
	//Take a proposed step through space
	integrateMotion(player.getPhysicsBody(), deltaTime);

	//Check for collisions in new step. Decide on new position and velocity if necessary
	handleCollisions(player, gameWorld);

	//Integrate acceleration
	integrateAcceleration(player.getPhysicsBody(), deltaTime);

	std::cout << "FINAL_STATE" << std::endl;
	player.getPhysicsBody().printState();
}

void GamePhysics::calculatePlayerForces(PlayerCharacter& player, const InputState& inputs, float deltaTime) {
	//In the future this may take into account more factors like a slow state of the player, or something like that.
	//Idea: maybe the InputHandler class should maintain a vector of the user's directional inputs. This would make it easy to add controller support at some point.
	bool pressingVert = inputs.pressingUp || inputs.pressingDown;
	bool pressingHori = inputs.pressingLeft || inputs.pressingRight;
	bool normalize = pressingVert && pressingHori;

	//calculate x and y acceleration: 
	if (normalize) {
		player.getPhysicsBody().xAccel = (inputs.pressingRight - inputs.pressingLeft) * player.readPlayerState().enginePower * deltaTime / (player.getPhysicsBody().mass * SQRT2);
		player.getPhysicsBody().yAccel = (inputs.pressingUp - inputs.pressingDown) * player.readPlayerState().enginePower * deltaTime / (player.getPhysicsBody().mass * SQRT2);
	} else {
		player.getPhysicsBody().xAccel = (inputs.pressingRight - inputs.pressingLeft) * player.readPlayerState().enginePower * deltaTime / player.getPhysicsBody().mass;
		player.getPhysicsBody().yAccel = (inputs.pressingUp - inputs.pressingDown) * player.readPlayerState().enginePower * deltaTime / player.getPhysicsBody().mass;
	}
	player.getPhysicsBody().xAccel -= player.getPhysicsBody().xVel * WATER_DRAG_FACTOR;
	player.getPhysicsBody().yAccel -= player.getPhysicsBody().yVel * WATER_DRAG_FACTOR;
	std::cout << "GamePhysics::calculatePlayerForces():" << std::endl;
	player.getPhysicsBody().printState();
}

void GamePhysics::integrateMotion(PhysicsBody& body, float deltaTime) {
	body.prevX = body.x;
	body.prevY = body.y;
	body.x += body.xVel * deltaTime;
	body.y += body.yVel * deltaTime;

	std::cout << "GamePhysics::IntegrateMotions():" << std::endl;
	body.printState();
}

void GamePhysics::integrateAcceleration(PhysicsBody& body, float deltaTime) {
	body.xVel += body.xAccel * deltaTime;
	body.yVel += body.yAccel * deltaTime;
	//body.xAccel = 0;
	//body.yAccel = 0;
	std::cout << "GamePhysics::integrateAcceleration():" << std::endl;
	body.printState();
}

void GamePhysics::handleCollisions(PlayerCharacter& player, int gameWorld) {
	std::cout << "GamePhysics::handleCollisions():" << std::endl;
	player.getPhysicsBody().printState();
	return;
}