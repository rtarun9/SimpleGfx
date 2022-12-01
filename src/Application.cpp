#include "Application.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <iostream>
#include <source_location>
#include <format>

namespace sgfx
{
	inline void fatalError(const std::string_view message, const std::source_location source_location = std::source_location::current())
	{
		const std::string errorMessage =
			std::string(message.data() +
				std::format(" Source Location data : File Name -> {}, Function Name -> {}, Line Number -> {}, Column -> {}", source_location.file_name(), source_location.function_name(), source_location.line(), source_location.column()));

		throw std::runtime_error(errorMessage.data());
	}

	Application::Application(const std::string_view windowTitle)
		: m_windowTitle(windowTitle)
	{
	}

	Application::~Application()
	{
		cleanup();
	}

	void Application::run()
	{
		init();

		loadContent();

		bool quit = false;
		while (!quit)
		{
			SDL_Event event{};
			while (SDL_PollEvent(&event))
			{
				if (event.type == SDL_QUIT)
				{
					quit = true;
				}

				const uint8_t* keyboardState = SDL_GetKeyboardState(nullptr);
				if (keyboardState[SDL_SCANCODE_ESCAPE])
				{
					quit = true;
				}
			}

			update();
			render();
		}
	}

	void Application::init()
	{
		// Initialize SDL2 and create window.
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			fatalError("Failed to initialize SDL2.");
		}

		// Get monitor dimensions.
		SDL_DisplayMode displayMode{};
		if (SDL_GetCurrentDisplayMode(0, &displayMode) < 0)
		{
			fatalError("Failed to get display mode.");
		}

		const uint32_t monitorWidth = displayMode.w;
		const uint32_t monitorHeight = displayMode.h;

		// Window must cover 90% of the screen.
		const uint32_t windowWidth = static_cast<uint32_t>(monitorWidth * 0.90f);
		const uint32_t windowHeight = static_cast<uint32_t>(monitorHeight * 0.90f);

		// Not made const as SDL_DestroyWindow requires us to pass a non - const SDL_Window.
		SDL_Window* m_window = SDL_CreateWindow("LunarEngine",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			windowWidth,
			windowHeight,
			SDL_WINDOW_ALLOW_HIGHDPI);

		if (!m_window)
		{
			fatalError("Failed to create SDL2 window.");
		}
	}

	void Application::cleanup()
	{
		SDL_DestroyWindow(m_window);
		SDL_Quit();
	}
}

