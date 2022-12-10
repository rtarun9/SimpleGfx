#pragma once

struct SDL_Window;

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

		void bindPipeline(const GraphicsPipeline& pipeline);

		[[nodiscard]] Microsoft::WRL::ComPtr<ID3D11VertexShader> createVertexShader(const std::wstring_view shaderPath, Microsoft::WRL::ComPtr<ID3DBlob>& outShaderBlob);
		[[nodiscard]] Microsoft::WRL::ComPtr<ID3D11PixelShader> createPixelShader(const std::wstring_view shaderPath);

		[[nodiscard]] Microsoft::WRL::ComPtr<ID3D11InputLayout> createInputLayout(ID3DBlob* vertexShaderBlob, std::span<const InputLayoutElementDesc> inputLayoutElementDescs);

		[[nodiscard]] GraphicsPipeline createGraphicsPipeline(const GraphicsPipelineCreationDesc& pipelineCreationDesc);

		template <typename T>
		[[nodiscard]] Microsoft::WRL::ComPtr<ID3D11Buffer> createBuffer(const BufferCreationDesc& bufferCreationDesc, std::span<const T> data = {});
	
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
		comptr<IDXGIFactory6> m_factory{};
		comptr<IDXGISwapChain1> m_swapchain{};
		comptr<ID3D11RenderTargetView> m_renderTargetView{};
	
		D3D11_VIEWPORT m_viewport{};
	};

	template<typename T>
	inline Microsoft::WRL::ComPtr<ID3D11Buffer> Application::createBuffer(const BufferCreationDesc& bufferCreationDesc, std::span<const T> data)
	{
		comptr<ID3D11Buffer> buffer{};

		const D3D11_BUFFER_DESC bufferDesc = 
		{
			.ByteWidth = static_cast<uint32_t>(data.size_bytes()),
			.Usage = bufferCreationDesc.usage,
			.BindFlags = bufferCreationDesc.bindFlags,
		};

		if (data.size() != 0)
		{
			const D3D11_SUBRESOURCE_DATA resourceData =
			{
				.pSysMem = data.data()
			};

			throwIfFailed(m_device->CreateBuffer(&bufferDesc, &resourceData, &buffer));
		}
		else
		{
			// Some stuff here ? 
		}

		return buffer;
	}
}