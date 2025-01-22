#pragma once

#include "pch.h"

class Window {
public:
	Window(const std::string_view& title, int width, int height);
	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	inline GLFWwindow* GetContext() const { return ctx; }

public:
	GLFWwindow* ctx;

	int width;
	int height;
};
