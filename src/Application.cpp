#include "Pch.hpp"

#include "Application.hpp"

#include <SDL.h>
#include <SDL_syswm.h>

#include <DirectXTex.h>

namespace sgfx
{
    Application::Application(const std::string_view windowTitle) : m_windowTitle(windowTitle) {}

    Application::~Application() { cleanup(); }

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
        SDL_Window* m_window = SDL_CreateWindow("LunarEngine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_windowWidth, m_windowHeight, SDL_WINDOW_ALLOW_HIGHDPI);

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

        // Create the fallback texture that will be used if some texture does not exist but the shader requires something to be bound at that slot.
        m_fallbackTexture = createTexture(L"assets/textures/Default.png");
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

        // Get the adapter with best performance.
        comptr<IDXGIAdapter1> adapter{};
        throwIfFailed(m_factory->EnumAdapterByGpuPreference(0u, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)));

        uint32_t deviceCreationFlags = 0u;

        if constexpr (SGFX_DEBUG)
        {
            deviceCreationFlags |= D3D11_CREATE_DEVICE_DEBUG;

            // Display chosen adapter.
            DXGI_ADAPTER_DESC1 adapterDesc{};
            throwIfFailed(adapter->GetDesc1(&adapterDesc));

            std::cout << "Chosen Adapter : ";
            std::wcout << adapterDesc.Description << L'\n';
        }

        // Create the D3D11 device and device context.
        throwIfFailed(
            ::D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, deviceCreationFlags, nullptr, 0u, D3D11_SDK_VERSION, &m_device, nullptr, &m_deviceContext));

        if constexpr (SGFX_DEBUG)
        {
            // Enable debug layer.
            throwIfFailed(m_device.As(&m_debug));

            // Setup info queue.
            throwIfFailed(m_device.As(&m_infoQueue));
            throwIfFailed(m_infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true));
            throwIfFailed(m_infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true));
            throwIfFailed(m_infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true));
        }
    }

    void Application::createSwapchainResources()
    {
        // Create the swapchain.
        const DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
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

        m_viewport = {
            .TopLeftX = 0.0f,
            .TopLeftY = 0.0f,
            .Width = static_cast<float>(m_windowWidth),
            .Height = static_cast<float>(m_windowHeight),
            .MinDepth = 0.0f,
            .MaxDepth = 1.0f,
        };
    }

    Microsoft::WRL::ComPtr<ID3D11VertexShader> Application::createVertexShader(const std::wstring_view shaderPath, comptr<ID3DBlob>& outShaderBlob)
    {
        comptr<ID3D11VertexShader> vertexShader{};

        comptr<ID3DBlob> errorBlob{};

        if (FAILED(::D3DCompileFromFile(shaderPath.data(), nullptr, nullptr, "VsMain", "vs_5_0", 0u, 0u, &outShaderBlob, &errorBlob)))
        {
            std::wcout << "Error in compiling shader : " << shaderPath << ". Error : " << static_cast<const char*>(errorBlob->GetBufferPointer());
            throw std::runtime_error("Shader compilation error.");
        }

        throwIfFailed(m_device->CreateVertexShader(outShaderBlob->GetBufferPointer(), outShaderBlob->GetBufferSize(), nullptr, &vertexShader));

        return vertexShader;
    }

    void Application::bindPipeline(const GraphicsPipeline& pipeline)
    {
        m_deviceContext->IASetPrimitiveTopology(pipeline.primitiveTopology);
        m_deviceContext->IASetInputLayout(pipeline.inputLayout.Get());

        m_deviceContext->VSSetShader(pipeline.vertexShader.Get(), nullptr, 0u);
        m_deviceContext->PSSetShader(pipeline.pixelShader.Get(), nullptr, 0u);
    }

    void Application::bindTexturePS(ID3D11ShaderResourceView* const srv, const uint32_t bindSlot)
    {
        if (!srv)
        {
            m_deviceContext->PSSetShaderResources(bindSlot, 1u, m_fallbackTexture.GetAddressOf());
        }
        else
        {
            m_deviceContext->PSSetShaderResources(bindSlot, 1u, &srv);
        }
    }

    Microsoft::WRL::ComPtr<ID3D11PixelShader> Application::createPixelShader(const std::wstring_view shaderPath)
    {
        comptr<ID3D11PixelShader> pixelShader{};

        comptr<ID3DBlob> shaderBlob{};
        comptr<ID3DBlob> errorBlob{};

        if (FAILED(::D3DCompileFromFile(shaderPath.data(), nullptr, nullptr, "PsMain", "ps_5_0", 0u, 0u, &shaderBlob, &errorBlob)))
        {
            std::wcout << "Error in compiling shader : " << shaderPath << ". Error : " << static_cast<const char*>(errorBlob->GetBufferPointer());
            throw std::runtime_error("Shader compilation error.");
        }

        throwIfFailed(m_device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &pixelShader));

        return pixelShader;
    }

    Microsoft::WRL::ComPtr<ID3D11InputLayout> Application::createInputLayout(ID3DBlob* vertexShaderBlob, std::span<const InputLayoutElementDesc> inputLayoutElementDescs)
    {
        comptr<ID3D11InputLayout> inputLayout{};

        std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementDescs{};
        inputElementDescs.reserve(inputLayoutElementDescs.size());

        for (const auto& inputLayoutElementDesc : inputLayoutElementDescs)
        {
            inputElementDescs.emplace_back(D3D11_INPUT_ELEMENT_DESC{
                .SemanticName = inputLayoutElementDesc.semanticName.c_str(),
                .SemanticIndex = 0u,
                .Format = inputLayoutElementDesc.format,
                .InputSlot = 0u,
                .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
                .InputSlotClass = inputLayoutElementDesc.inputClassification,
                .InstanceDataStepRate = 0u,
            });
        }

        throwIfFailed(m_device->CreateInputLayout(inputElementDescs.data(),
                                                  static_cast<uint32_t>(inputElementDescs.size()),
                                                  vertexShaderBlob->GetBufferPointer(),
                                                  vertexShaderBlob->GetBufferSize(),
                                                  &inputLayout));

        return inputLayout;
    }

    GraphicsPipeline Application::createGraphicsPipeline(const GraphicsPipelineCreationDesc& pipelineCreationDesc)
    {
        comptr<ID3DBlob> vertexShaderBlob{};

        comptr<ID3D11VertexShader> vertexShader = createVertexShader(pipelineCreationDesc.vertexShaderPath, vertexShaderBlob);

        return GraphicsPipeline{
            .vertexShader = vertexShader,
            .pixelShader = createPixelShader(pipelineCreationDesc.pixelShaderPath),
            .inputLayout = createInputLayout(vertexShaderBlob.Get(), pipelineCreationDesc.inputLayoutElements),
            .primitiveTopology = pipelineCreationDesc.primitiveTopology,
            .vertexSize = pipelineCreationDesc.vertexSize,
        };
    }

    wrl::ComPtr<ID3D11ShaderResourceView> Application::createTexture(const std::wstring_view texturePath)
    {
        // Main reference : https://github.com/GraphicsProgramming/learnd3d11/blob/main/src/Cpp/1-getting-started/1-3-1-Texturing/TexturingApplication.cpp.

        comptr<ID3D11ShaderResourceView> srv{};

        DirectX::TexMetadata metaData{};
        DirectX::ScratchImage scratchImage{};

        if (FAILED(DirectX::LoadFromWICFile(texturePath.data(), DirectX::WIC_FLAGS_NONE, &metaData, scratchImage)))
        {
            std::wcout << L"Failed to load texture from path : " << texturePath << L'\n';
            throw std::runtime_error("Texture Loading Error");
        }

        DirectX::ScratchImage mipChain{};
        throwIfFailed(DirectX::GenerateMipMaps(scratchImage.GetImages(), scratchImage.GetImageCount(), metaData, DirectX::TEX_FILTER_DEFAULT, 0u, mipChain));

        comptr<ID3D11Resource> texture{};
        throwIfFailed(DirectX::CreateTexture(m_device.Get(), mipChain.GetImages(), mipChain.GetImageCount(), mipChain.GetMetadata(), &texture));

        throwIfFailed(DirectX::CreateShaderResourceView(m_device.Get(), mipChain.GetImages(), mipChain.GetImageCount(), mipChain.GetMetadata(), &srv));

        return srv;
    }

    wrl::ComPtr<ID3D11SamplerState> Application::createSampler(const SamplerCreationDesc& samplerCreationDesc)
    {
        comptr<ID3D11SamplerState> sampler{};

        const D3D11_SAMPLER_DESC samplerDesc = {
            .Filter = samplerCreationDesc.filter,
            .AddressU = samplerCreationDesc.addressMode,
            .AddressV = samplerCreationDesc.addressMode,
            .AddressW = samplerCreationDesc.addressMode,
            .MaxAnisotropy = D3D11_MAX_MAXANISOTROPY,
            .ComparisonFunc = D3D11_COMPARISON_NEVER,
        };

        throwIfFailed(m_device->CreateSamplerState(&samplerDesc, &sampler));

        return sampler;
    }
}
