#pragma once

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <string>

class Window {
private:
	GLFWwindow* windowGL;
	int windowHeight;
	int screenHeight;
	std::string title;

public:
	Window();
	~Window();

	void createWindow(std::string title, int width, int height) {};

	void gameLoop() {};

	GLFWwindow* getWindow() const { return windowGL; }

};