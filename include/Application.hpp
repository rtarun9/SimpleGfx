#pragma once

#include "Camera.hpp"
#include "Model.hpp"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_sdl.h>

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
        virtual void update(const float deltaTime) = 0;
        virtual void render() = 0;

        template <typename T> void updateConstantBuffer(ConstantBuffer<T>& buffer) const;

        void bindPipeline(const GraphicsPipeline& pipeline);
        void bindTexturePS(ID3D11ShaderResourceView* const srv, const uint32_t bindSlot);

        [[nodiscard]] wrl::ComPtr<ID3D11VertexShader> createVertexShader(const std::wstring_view shaderPath, wrl::ComPtr<ID3DBlob>& outShaderBlob);
        [[nodiscard]] wrl::ComPtr<ID3D11PixelShader> createPixelShader(const std::wstring_view shaderPath);

        [[nodiscard]] wrl::ComPtr<ID3D11InputLayout> createInputLayout(ID3DBlob* const vertexShaderBlob, std::span<const InputLayoutElementDesc> inputLayoutElementDescs);

        [[nodiscard]] GraphicsPipeline createGraphicsPipeline(const GraphicsPipelineCreationDesc& pipelineCreationDesc);

        [[nodiscard]] wrl::ComPtr<ID3D11ShaderResourceView> createTexture(const std::wstring_view texturePath);
        [[nodiscard]] wrl::ComPtr<ID3D11SamplerState> createSampler(const SamplerCreationDesc& samplerCreationDesc);
        
        [[nodiscard]] RenderTarget createRenderTarget(const uint32_t width, const uint32_t height, const DXGI_FORMAT format);

        [[nodiscard]] Model createModel(const std::string_view modelPath);

        [[nodiscard]] wrl::ComPtr<ID3D11DepthStencilView> createDepthStencilView();

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

        Camera m_camera{};

        // Default / Fallback resources.
        comptr<ID3D11ShaderResourceView> m_fallbackTexture{};
    };

    template <typename T> inline void Application::updateConstantBuffer(ConstantBuffer<T>& buffer) const
    {
        m_deviceContext->UpdateSubresource(buffer.buffer.Get(), 0u, nullptr, &buffer.data, 0u, 0u);
    }

    template <typename T> inline wrl::ComPtr<ID3D11Buffer> Application::createBuffer(const BufferCreationDesc& bufferCreationDesc, std::span<const T> data)
    {
        comptr<ID3D11Buffer> buffer{};

        if (data.size() != 0)
        {
            const D3D11_BUFFER_DESC bufferDesc = {
                .ByteWidth = static_cast<uint32_t>(data.size_bytes()),
                .Usage = bufferCreationDesc.usage,
                .BindFlags = bufferCreationDesc.bindFlags,
            };

            const D3D11_SUBRESOURCE_DATA resourceData = {.pSysMem = data.data()};

            throwIfFailed(m_device->CreateBuffer(&bufferDesc, &resourceData, &buffer));
        }
        else
        {
            // If the code reaches here, this buffer is a constant buffer.
            const D3D11_BUFFER_DESC bufferDesc = {
                .ByteWidth = static_cast<uint32_t>(sizeof(T)),
                .Usage = bufferCreationDesc.usage,
                .BindFlags = bufferCreationDesc.bindFlags,
                .CPUAccessFlags = 0u,
            };

            throwIfFailed(m_device->CreateBuffer(&bufferDesc, nullptr, &buffer));
        }

        return buffer;
    }
}