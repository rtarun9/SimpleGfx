#pragma once

#include <string_view>

#include <d3d11.h>
#include <dxgi1_3.h>
#include <wrl.h>

#include <source_location>
#include <format>

struct SDL_Window;

inline void fatalError(const std::string_view message, const std::source_location sourceLocation = std::source_location::current())
{
	const std::string errorMessage =
		std::string(message.data() +
			std::format(" Source Location data : File Name -> {}, Function Name -> {}, Line Number -> {}, Column -> {}", sourceLocation.file_name(), sourceLocation.function_name(), sourceLocation.line(), sourceLocation.column()));

	throw std::runtime_error(errorMessage.data());
}

inline void throwIfFailed(const HRESULT hr, const std::source_location sourceLocation = std::source_location::current())
{
	if (FAILED(hr))
	{
		fatalError("HRESULT failed!", sourceLocation);
	}
}

namespace sgfx
{
#ifdef _DEBUG
	constexpr bool SGFX_DEBUG = true;
#else
	constexpr bool SGFX_DEBUG = false;
#endif

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
		virtual void update() = 0;
		virtual void render() = 0;

	private:
		void createDeviceResources();
		void createSwapchainResources();

	protected:
		template<typename T>
		using comptr = Microsoft::WRL::ComPtr<T>;

		HWND m_windowHandle{};
		SDL_Window* m_window{};

		uint32_t m_windowWidth{};
		uint32_t m_windowHeight{};

		std::string m_windowTitle{};

		comptr<ID3D11Device> m_device{};
		comptr<ID3D11DeviceContext> m_deviceContext{};
		comptr<IDXGIFactory3> m_factory{};
		comptr<IDXGISwapChain1> m_swapchain{};
		comptr<ID3D11RenderTargetView> m_renderTargetView{};
	
		D3D11_VIEWPORT m_viewport{};
	};
}