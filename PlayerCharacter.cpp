#include "PlayerCharacter.h"

PlayerCharacter::PlayerCharacter() {};

PlayerCharacter::PlayerCharacter(float x, float y, float xVel, float yVel, float halfWidth, float halfHeight, float mass, float enginePower, float maxHealth, float maxFuelLevel) {
	physicsBody.x = x;
	physicsBody.y = y;
	physicsBody.prevX = x;
	physicsBody.prevY = y;
	physicsBody.xAccel = 0;
	physicsBody.yAccel = 0;
	physicsBody.xVel = xVel;
	physicsBody.yVel = yVel;
	physicsBody.halfWidth = halfWidth;
	physicsBody.halfHeight = halfHeight;
	physicsBody.mass = mass;

	playerState.enginePower = enginePower;
	playerState.maxHealth = maxHealth;
	playerState.health = maxHealth;
	playerState.maxFuelLevel = maxFuelLevel;
	playerState.fuelLevel = maxFuelLevel;
}

PlayerCharacter::PlayerCharacter(PhysicsBody body, PlayerState state) {
	physicsBody = body;
	playerState = state;
}

PhysicsBody& PlayerCharacter::getPhysicsBody() {
	return physicsBody;
}

PlayerState& PlayerCharacter::getPlayerState() {
	return playerState;
}

const PlayerState& PlayerCharacter::readPlayerState() {
	return playerState;
}

