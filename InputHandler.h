#pragma once

struct InputState {
	//Cardinal directions
	bool pressingRight = false;
	float pressingRightTime = 0;

	bool pressingLeft = false;
	float pressingLeftTime = 0;

	bool pressingUp = false;
	float pressingUpTime = 0;

	bool pressingDown = false;
	float pressingDownTime = 0;

	//Mouse inputs
	int mouseX = 0;
	int mouseY = 0;

	bool leftClicking = false;
	float leftClickingTime = 0;

	bool rightClicking = false;
	float rightClickingTime = 0;

	//Other inputs
	bool pressingQ = false;
	bool pressingEsc = false;
	bool pressingQuit = false;
	bool pressingDebug = false;
};

class InputHandler {
public:
	InputHandler();

	//Call once per frame, will check all inputs and update internal state.
	void pollInputs(float frameDelta);

	const InputState& getState() const;
	
	void printState();

private:
	InputState currentState;
};