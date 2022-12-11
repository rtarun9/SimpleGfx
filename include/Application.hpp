#pragma once

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
        virtual void update() = 0;
        virtual void render() = 0;

        void bindPipeline(const GraphicsPipeline& pipeline);
        void bindTexturePS(ID3D11ShaderResourceView* const srv, const uint32_t bindSlot);

        [[nodiscard]] wrl::ComPtr<ID3D11VertexShader> createVertexShader(const std::wstring_view shaderPath, wrl::ComPtr<ID3DBlob>& outShaderBlob);
        [[nodiscard]] wrl::ComPtr<ID3D11PixelShader> createPixelShader(const std::wstring_view shaderPath);

        [[nodiscard]] wrl::ComPtr<ID3D11InputLayout> createInputLayout(ID3DBlob* const vertexShaderBlob, std::span<const InputLayoutElementDesc> inputLayoutElementDescs);

        [[nodiscard]] GraphicsPipeline createGraphicsPipeline(const GraphicsPipelineCreationDesc& pipelineCreationDesc);

        [[nodiscard]] wrl::ComPtr<ID3D11ShaderResourceView> createTexture(const std::wstring_view texturePath);
        [[nodiscard]] wrl::ComPtr<ID3D11SamplerState> createSampler(const SamplerCreationDesc& samplerCreationDesc);

        template <typename T> [[nodiscard]] wrl::ComPtr<ID3D11Buffer> createBuffer(const BufferCreationDesc& bufferCreationDesc, std::span<const T> data = {});


      private:
        void createDeviceResources();
        void createSwapchainResources();

      protected:
        template <typename T> using comptr = Microsoft::WRL::ComPtr<T>;

        HWND m_windowHandle{};
        SDL_Window* m_window{};

        uint32_t m_windowWidth{};
        uint32_t m_windowHeight{};

        std::string m_windowTitle{};

        comptr<ID3D11Device> m_device{};
        comptr<ID3D11Debug> m_debug{};
        comptr<ID3D11InfoQueue> m_infoQueue{};
        comptr<ID3D11DeviceContext> m_deviceContext{};
        comptr<IDXGIFactory6> m_factory{};
        comptr<IDXGISwapChain1> m_swapchain{};
        comptr<ID3D11RenderTargetView> m_renderTargetView{};

        D3D11_VIEWPORT m_viewport{};

        // Default / Fallback resources.
        comptr<ID3D11ShaderResourceView> m_fallbackTexture{};
    };

    template <typename T> inline wrl::ComPtr<ID3D11Buffer> Application::createBuffer(const BufferCreationDesc& bufferCreationDesc, std::span<const T> data)
    {
        comptr<ID3D11Buffer> buffer{};

        const D3D11_BUFFER_DESC bufferDesc = {
            .ByteWidth = static_cast<uint32_t>(data.size_bytes()),
            .Usage = bufferCreationDesc.usage,
            .BindFlags = bufferCreationDesc.bindFlags,
        };

        if (data.size() != 0)
        {
            const D3D11_SUBRESOURCE_DATA resourceData = {.pSysMem = data.data()};

            throwIfFailed(m_device->CreateBuffer(&bufferDesc, &resourceData, &buffer));
        }
        else
        {
            // Some stuff here ?
        }

        return buffer;
    }
}