#pragma once
#include "PlayerCharacter.h"
#include "PhysicsBody.h"
#include "InputHandler.h"
//This class will handle all physics updates for entities in the world. It will take in factors like player inputs and state, AI, worldstate (collisions)
//	and will update the positions and velocities of each object. This isnt great because then the physics class will have to take in many many things from many different peopole. It will essentially have the whole AI system integrated within it.
//	eh whatever, we can always refactor this later

class GamePhysics {
public:
	GamePhysics();
	void updatePlayerPosition(PlayerCharacter& player, const InputState& inputs, int gameWorld, float deltaTime);
private:
	//physics constants
	static constexpr float GRAVITY_ACCEL = 9.8f;
	static constexpr float WATER_DRAG_FACTOR = 0.005;
	static constexpr float SQRT2 = 1.41421356237f;

	void handleCollisions(PlayerCharacter& player, int gameWorld);
	void integrateMotion(PhysicsBody& body, float deltaTime);
	void integrateAcceleration(PhysicsBody& body, float deltaTime);
	void calculatePlayerForces(PlayerCharacter& player, const InputState& inputs, float deltaTime);
};