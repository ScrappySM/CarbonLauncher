#pragma once

#pragma comment(lib, "dwmapi.lib")

#define WIN32_LEAN_AND_MEAN
#define GLFW_INCLUDE_NONE
#define JSON_NOEXCEPTION

#include <Windows.h>
#include <TlHelp32.h>
#include <dwmapi.h>
#include <shellapi.h>

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <mutex>
#include <unordered_map>

#include <imgui.h>
#include <imgui_internal.h>

#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <spdlog/spdlog.h>
#include <fmt/format.h>

#include <nlohmann/json.hpp>

#include <cpr/cpr.h>
