#pragma once
#include "PhysicsBody.h"

struct PlayerState {
	float enginePower = 0;

	float health = 0;
	float maxHealth = 0;

	float fuelLevel = 0;
	float maxFuelLevel = 0;

	//float shieldLevel = 0;
	//float maxShieldLevel = 0;
};


class PlayerCharacter {
public:
	PlayerCharacter();
	PlayerCharacter(float x, float y, float xVel, float yVel, float halfWidth, float halfHeight, float mass, float enginePower, float maxHealth, float maxFuelLevel);
	PlayerCharacter(PhysicsBody body, PlayerState state);
	PhysicsBody& getPhysicsBody();

	const PlayerState& readPlayerState();
	PlayerState& getPlayerState();
private:
	PhysicsBody physicsBody;
	PlayerState playerState;
};