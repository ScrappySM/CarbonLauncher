#include "pch.h"

#include <font/IconsFontAwesome6.h>
#include <font/IconsFontAwesome6.h_fa-solid-900.ttf.h>

#include "window.h"

Window::Window(const std::string_view& title, int width, int height) : width(width), height(height) {
	ZoneScoped;

	if (!glfwInit()) {
		spdlog::error("Failed to initialize GLFW");
		std::exit(EXIT_FAILURE);
	}

	glfwSetErrorCallback([](int error, const char* description) {
		spdlog::error("GLFW Error {}: {}", error, description);
		});

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	this->ctx = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);

	if (!this->ctx) {
		spdlog::error("Failed to create window");

		glfwTerminate();

		std::exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(this->ctx);

	glfwSwapInterval(1); // V-Sync

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		spdlog::error("Failed to initialize GLAD");
		glfwTerminate();
		std::exit(EXIT_FAILURE);
	}

	glfwSetWindowUserPointer(this->ctx, this);
	glfwSetFramebufferSizeCallback(this->ctx, [](GLFWwindow* window, int width, int height) {
		Window* instance = (Window*)glfwGetWindowUserPointer(window);
		instance->width = width;
		instance->height = height;
		});

	TracyGpuContext;
	TracyGpuContextName("Main", 4);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(this->ctx, true);
	ImGui_ImplOpenGL3_Init("#version 460");

	ImGuiStyle& style = ImGui::GetStyle();

	// Rounded corners
	style.WindowRounding = 4.0f;
	style.FrameRounding = 4.0f;
	style.GrabRounding = 4.0f;
	style.PopupRounding = 4.0f;
	style.ScrollbarRounding = 4.0f;
	style.TabRounding = 4.0f;
	style.ChildRounding = 4.0f;

	for (int i = 0; i < ImGuiCol_COUNT; i++) {
		ImVec4* col = &style.Colors[i];
		float h, s, v;

		ImGui::ColorConvertRGBtoHSV(col->x, col->y, col->z, h, s, v);

		if (h > 0.59 && h < 0.61) {
			h = 0.0f;
			s = 0.5f;
		}

		ImGui::ColorConvertHSVtoRGB(h, s, v, col->x, col->y, col->z);
	}

	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	constexpr float fontSize = 18.0f;
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\seguisb.ttf", fontSize);

	constexpr float iconFontSize = fontSize * 2.0f / 3.0f;
	void* data = (void*)s_fa_solid_900_ttf;
	int size = sizeof(s_fa_solid_900_ttf);

	static const ImWchar iconsRanges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
	ImFontConfig iconsConfig;
	iconsConfig.MergeMode = true;
	iconsConfig.PixelSnapH = true;
	iconsConfig.GlyphMinAdvanceX = iconFontSize;
	iconsConfig.FontDataOwnedByAtlas = false;
	io.Fonts->AddFontFromMemoryTTF(data, size, iconFontSize, &iconsConfig, iconsRanges);
	io.Fonts->Build();
}

Window::~Window() {
	ZoneScoped;

	glfwDestroyWindow(this->ctx);
	glfwTerminate();
}