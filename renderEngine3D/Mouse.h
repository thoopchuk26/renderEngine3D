#pragma once

#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <map>

class KeyInput {
private:
	bool isEnabled;
	std::map<int, bool> keys;
	void setIsKeyDown(int key, bool isDown);

public:
	KeyInput(std::vector<int> keysToMonitor);
	~KeyInput();

	bool getIsKeyDown(int key);
	bool getIsEnabled() {};
	void setIsEnabled(bool value) {};

}