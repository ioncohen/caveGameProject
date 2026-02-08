#include "InputHandler.h"
#include <SDL.h>
#include <iostream>

InputHandler::InputHandler() {
	pollInputs(1);
}

static void updateDurationAndState(bool newState, bool& structState, float& structDuration, float deltaTime) {
	if (newState && !structState) {
		structDuration = 0;
		structState = true;
	}
	else if (newState && structState) {
		structDuration += deltaTime;
	}
	else if (!newState) {
		structDuration = -1;
		structState = false;
	}
}

static void updateDurations(bool newState, float& duration, float deltaTime) {
	if (newState && duration >= 0) {
		duration += deltaTime;
	}
	else if (newState) {
		duration = 0;
	}
	else {
		duration = -1;
	}
}

void InputHandler::pollInputs(float deltaTime) {
	//new formulation, since we can only get events for keys
	//we have a current state with states and times
	//we look for updates, process them to decide a new set of states
	//we loop through the times and update them based on the states.
	//we probably have to get rid of "updateDurationAndState"


	Uint32 mouseButtons = SDL_GetMouseState(&currentState.mouseX, &currentState.mouseY);
	currentState.leftClicking = mouseButtons & SDL_BUTTON(SDL_BUTTON_LEFT);
	currentState.rightClicking = mouseButtons & SDL_BUTTON(SDL_BUTTON_RIGHT);
	
	SDL_Event event;
	while (SDL_PollEvent(&event) != 0) {
		if (event.type == SDL_QUIT) {
			currentState.pressingQuit = true;
			break;
		}
		if (event.type == SDL_KEYDOWN) {
			switch (event.key.keysym.sym) {
			case SDLK_0:
				currentState.pressingDebug = true;
				break;
			case SDLK_w:
			case SDLK_UP:
				currentState.pressingUp = true;
				break;
			case SDLK_s:
			case SDLK_DOWN:
				currentState.pressingDown = true;
				break;
			case SDLK_a:
			case SDLK_LEFT:
				currentState.pressingLeft = true;
				break;
			case SDLK_d:
			case SDLK_RIGHT:
				currentState.pressingRight = true;
				break;
			case SDLK_q:
				currentState.pressingQ = true;
				break;
			default:
				break;
			}
		}
		if (event.type == SDL_KEYUP) {
			switch (event.key.keysym.sym) {
			case SDLK_0:
				currentState.pressingDebug = false;
				break;
			case SDLK_w:
			case SDLK_UP:
				currentState.pressingUp = false;
				break;
			case SDLK_s:
			case SDLK_DOWN:
				currentState.pressingDown = false;
				break;
			case SDLK_a:
			case SDLK_LEFT:
				currentState.pressingLeft = false;
				break;
			case SDLK_d:
			case SDLK_RIGHT:
				currentState.pressingRight = false;
				break;
			case SDLK_q:
				currentState.pressingQ = false;
				break;
			default:
				break;
			}
		}
	}
	updateDurations(currentState.pressingRight, currentState.pressingRightTime, deltaTime);
	updateDurations(currentState.pressingLeft, currentState.pressingLeftTime, deltaTime);
	updateDurations(currentState.pressingUp, currentState.pressingUpTime, deltaTime);
	updateDurations(currentState.pressingDown, currentState.pressingDownTime, deltaTime);
	updateDurations(currentState.leftClicking, currentState.leftClickingTime, deltaTime);
	updateDurations(currentState.rightClicking, currentState.rightClickingTime, deltaTime);

}

const InputState& InputHandler::getState() const {
	return currentState;
}

void InputHandler::printState() {
	std::cout << std::endl;
	std::cout << "--Input State--" << std::endl;
	std::cout << "pressingRight =" << currentState.pressingRight << std::endl;
	std::cout << "pressingRightTime =" << currentState.pressingRightTime << std::endl;

	std::cout << "pressingLeft =" << currentState.pressingLeft << std::endl;
	std::cout << "pressingLeftTime =" << currentState.pressingLeftTime << std::endl;

	std::cout << "pressingUp =" << currentState.pressingUp << std::endl;
	std::cout << "pressingUpTime =" << currentState.pressingUpTime << std::endl;

	std::cout << "pressingDown =" << currentState.pressingDown << std::endl;
	std::cout << "pressingDownTime =" << currentState.pressingDownTime << std::endl;

	std::cout << "mouseX =" << currentState.mouseX << std::endl;
	std::cout << "mouseY =" << currentState.mouseY << std::endl;
	std::cout << "leftClicking =" << currentState.leftClicking << std::endl;
	std::cout << "leftClickingTime =" << currentState.leftClickingTime << std::endl;

	std::cout << "rightClicking =" << currentState.rightClicking << std::endl;
	std::cout << "rightClickingTime =" << currentState.rightClickingTime << std::endl;

	//Other inputs
	std::cout << "pressingQ =" << currentState.pressingQ << std::endl;
	std::cout << "pressingEsc =" << currentState.pressingEsc << std::endl;
	std::cout << "pressingQuit =" << currentState.pressingQuit << std::endl;
	std::cout << "pressingDebug =" << currentState.pressingDebug << std::endl;
}