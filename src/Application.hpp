#pragma once

#include <string_view>

struct SDL_Window;

namespace sgfx
{
	class Application
	{
	public:
		Application(const std::string_view windowTitle);
		virtual ~Application();

		void run();

	protected:
		virtual void init();
		virtual void cleanup();

		virtual void loadContent() = 0;
		virtual void render() = 0;
		virtual void update() = 0;

	private:
		SDL_Window* m_window{};

		uint32_t m_windowWidth{};
		uint32_t m_windowHeight{};

		std::string m_windowTitle{};
	};
}