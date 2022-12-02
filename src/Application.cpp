#include "Application.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <iostream>
#include <exception>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dxguid.lib")

namespace sgfx
{
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
		try
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
		catch (const std::exception& exception)
		{
			std::cerr << exception.what() << "\n";
			return;
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
		m_windowWidth = static_cast<uint32_t>(monitorWidth * 0.90f);
		m_windowHeight = static_cast<uint32_t>(monitorHeight * 0.90f);

		// Not made const as SDL_DestroyWindow requires us to pass a non - const SDL_Window.
		SDL_Window* m_window = SDL_CreateWindow("LunarEngine",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			m_windowWidth,
			m_windowHeight,
			SDL_WINDOW_ALLOW_HIGHDPI);

		if (!m_window)
		{
			fatalError("Failed to create SDL2 window.");
		}

		SDL_SysWMinfo wmInfo{};
		SDL_VERSION(&wmInfo.version);

		SDL_GetWindowWMInfo(m_window, &wmInfo);
		m_windowHandle = wmInfo.info.win.window;

		// Initialize graphics back end.
		createDeviceResources();
		createSwapchainResources();
	}

	void Application::cleanup()
	{
		SDL_DestroyWindow(m_window);
		SDL_Quit();
	}

	void Application::createDeviceResources()
	{
		// Create the DXGI factory (with debug flags set in debug build).
		uint32_t factoryCreationFlags = 0u;
		if constexpr (SGFX_DEBUG)
		{
			factoryCreationFlags = DXGI_CREATE_FACTORY_DEBUG;
		}

		throwIfFailed(::CreateDXGIFactory2(factoryCreationFlags, IID_PPV_ARGS(&m_factory)));

		// Create the D3D11 device and device context.
		throwIfFailed(::D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0u, nullptr, 0u, D3D11_SDK_VERSION, &m_device, nullptr, &m_deviceContext));
	}

	void Application::createSwapchainResources()
	{
		// Create the swapchain.
		const DXGI_SWAP_CHAIN_DESC1 swapChainDesc =
		{
			.Width = m_windowWidth,
			.Height = m_windowHeight,
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.SampleDesc = {1u, 0u},
			.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
			.BufferCount = 2u,
			.Scaling = DXGI_SCALING_STRETCH,
			.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
			.Flags = 0u,
		};

		throwIfFailed(m_factory->CreateSwapChainForHwnd(m_device.Get(), m_windowHandle, &swapChainDesc, nullptr, nullptr, &m_swapchain));

		// Setup the swapchain backbuffer render target view.
		comptr<ID3D11Texture2D> backBuffer{};
		throwIfFailed(m_swapchain->GetBuffer(0u, IID_PPV_ARGS(&backBuffer)));

		throwIfFailed(m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_renderTargetView));

		m_viewport =
		{
			.TopLeftX = 0.0f,
			.TopLeftY = 0.0f,
			.Width = static_cast<float>(m_windowWidth),
			.Height = static_cast<float>(m_windowHeight),
			.MinDepth = 0.0f,
			.MaxDepth = 1.0f
		};
	}
}

