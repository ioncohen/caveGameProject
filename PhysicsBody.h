#pragma once
#include <iostream>

struct PhysicsBody {
	float x = 0;
	float y = 0;
	float prevX = 0;
	float prevY = 0;
	float xAccel = 0;
	float yAccel = 0;
	float xVel = 0;
	float yVel = 0;
	//need a physical rectangle, no?
	float halfWidth = 0;
	float halfHeight = 0;
	float mass = 1;

	void printState() const {
		std::cout << "--Printing PhysicsBody State---" << std::endl;
		std::cout << "Position: [" << x << ", " << y << "]" << std::endl;
		std::cout << "Prev Position: [" << prevX << ", " << prevY << "]" << std::endl;
		std::cout << "Accel: [" << xAccel << ", " << yAccel << "]" << std::endl;
		std::cout << "Vel: [" << xVel << ", " << yVel << "]" << std::endl;
		std::cout << "Dimensions: [" << halfWidth << ", " << halfHeight << "]" << std::endl;
		std::cout << std::endl;
	}
};