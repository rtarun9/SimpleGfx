#include "Pch.hpp"

#include "Engine.hpp"

Engine::Engine(const std::string_view windowTitle) : sgfx::Application(windowTitle) {}

void Engine::loadContent()
{
    // Wrap the createModel functions in their own jthreads for speed. Not a very elegant solution, in the future the application class will take care of this.

    std::jthread cube1Thread([&]() { m_renderables["cube"] = createModel("assets/models/Cube/glTF/Cube.gltf"); });

    std::jthread cube2Thread(
        [&]()
        {
            m_renderables["cube2"] = createModel("assets/models/Cube/glTF/Cube.gltf");

            m_renderables["cube2"].getTransformComponent()->translate = {5.0f, 0.0f, -2.0f};
        });

    std::jthread sponzaThread(
        [&]()
        {
            m_renderables["sponza"] = createModel("assets/models/sponza-gltf-pbr/sponza.glb");
            m_renderables["sponza"].getTransformComponent()->scale = {0.1f, 0.1f, 0.1f};
        });
    
    std::jthread scifiHelmetThread(
        [&]()
        {
            m_renderables["scifi-helmet"] = createModel("assets/models/SciFiHelmet/glTF/SciFiHelmet.gltf");
        });
   


    m_pipeline = createGraphicsPipeline(sgfx::GraphicsPipelineCreationDesc{
        .vertexShaderPath = L"shaders/PhongShader.hlsl",
        .pixelShaderPath = L"shaders/PhongShader.hlsl",
        .inputLayoutElements =
            {
                sgfx::InputLayoutElementDesc{.semanticName = "Position", .format = DXGI_FORMAT_R32G32B32_FLOAT, .inputClassification = D3D11_INPUT_PER_VERTEX_DATA},
                sgfx::InputLayoutElementDesc{.semanticName = "TextureCoord", .format = DXGI_FORMAT_R32G32_FLOAT, .inputClassification = D3D11_INPUT_PER_VERTEX_DATA},
                sgfx::InputLayoutElementDesc{.semanticName = "Normal", .format = DXGI_FORMAT_R32G32B32_FLOAT, .inputClassification = D3D11_INPUT_PER_VERTEX_DATA},
                sgfx::InputLayoutElementDesc{.semanticName = "Tangent", .format = DXGI_FORMAT_R32G32B32A32_FLOAT, .inputClassification = D3D11_INPUT_PER_VERTEX_DATA},
                sgfx::InputLayoutElementDesc{.semanticName = "BiTangent", .format = DXGI_FORMAT_R32G32B32_FLOAT, .inputClassification = D3D11_INPUT_PER_VERTEX_DATA},
            },
        .primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
        .vertexSize = sizeof(sgfx::ModelVertex),
    });

    m_lightPipeline = createGraphicsPipeline(sgfx::GraphicsPipelineCreationDesc{
        .vertexShaderPath = L"shaders/LightShader.hlsl",
        .pixelShaderPath = L"shaders/LightShader.hlsl",
        .inputLayoutElements =
            {
                sgfx::InputLayoutElementDesc{.semanticName = "Position", .format = DXGI_FORMAT_R32G32B32_FLOAT, .inputClassification = D3D11_INPUT_PER_VERTEX_DATA},
                sgfx::InputLayoutElementDesc{.semanticName = "TextureCoord", .format = DXGI_FORMAT_R32G32_FLOAT, .inputClassification = D3D11_INPUT_PER_VERTEX_DATA},
                sgfx::InputLayoutElementDesc{.semanticName = "Normal", .format = DXGI_FORMAT_R32G32B32_FLOAT, .inputClassification = D3D11_INPUT_PER_VERTEX_DATA},
                sgfx::InputLayoutElementDesc{.semanticName = "Tangent", .format = DXGI_FORMAT_R32G32B32A32_FLOAT, .inputClassification = D3D11_INPUT_PER_VERTEX_DATA},
                sgfx::InputLayoutElementDesc{.semanticName = "BiTangent", .format = DXGI_FORMAT_R32G32B32_FLOAT, .inputClassification = D3D11_INPUT_PER_VERTEX_DATA},
            },
        .primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
        .vertexSize = sizeof(sgfx::ModelVertex),
    });

    m_fullscreenPassPipeline = createGraphicsPipeline(sgfx::GraphicsPipelineCreationDesc{
        .vertexShaderPath = L"shaders/FullscreenPass.hlsl",
        .pixelShaderPath = L"shaders/FullscreenPass.hlsl",
        .primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    });

    m_sceneBuffer.buffer = createBuffer<sgfx::TransformBuffer>(sgfx::BufferCreationDesc{.usage = D3D11_USAGE_DEFAULT, .bindFlags = D3D11_BIND_CONSTANT_BUFFER});

    m_light = createModel("assets/models/Cube/glTF/Cube.gltf");
    m_light.getTransformComponent()->scale = {0.2f, 0.2f, 0.2f};
    m_light.getTransformComponent()->translate = {2.2f, 2.2f, -0.5f};

    m_dsv = createDepthStencilView();

    m_offscreenRT = createRenderTarget(m_windowWidth, m_windowHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);

    m_offscreenSampler = createSampler(sgfx::SamplerCreationDesc{
        .filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
        .addressMode = D3D11_TEXTURE_ADDRESS_WRAP,
    });
}

void Engine::update(const float deltaTime)
{
    m_camera.update(deltaTime);

    const math::XMMATRIX viewMatrix = m_camera.getLookAtMatrix();
    const math::XMMATRIX projectionMatrix = math::XMMatrixPerspectiveFovLH(math::XMConvertToRadians(45.0f), m_windowWidth / static_cast<float>(m_windowHeight), 0.1f, 1000.0f);

    m_sceneBuffer.data.viewMatrix = viewMatrix;
    m_sceneBuffer.data.viewProjectionMatrix = viewMatrix * projectionMatrix;

    const math::XMVECTOR lightPosition = math::XMLoadFloat3(&m_light.getTransformComponent()->translate);

    const math::XMVECTOR viewSpaceLightPosition = math::XMVector3TransformCoord(lightPosition, viewMatrix);

    math::XMStoreFloat3(&m_sceneBuffer.data.viewSpacePointLightPosition, viewSpaceLightPosition);

    m_sceneBuffer.data.cameraPosition = {m_camera.m_cameraPosition.x, m_camera.m_cameraPosition.y, m_camera.m_cameraPosition.z};

    updateConstantBuffer(m_sceneBuffer);

    for (auto& [name, renderable] : m_renderables)
    {
        renderable.updateTransformBuffer(viewMatrix, m_deviceContext.Get());
    }

    m_light.updateTransformBuffer(viewMatrix, m_deviceContext.Get());
}

void Engine::render()
{
    // Start the Dear ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Scene menu");
    ImGui::SliderFloat("camera mvmt speed", &m_camera.m_movementSpeed, 0.1f, 50.0f);
    ImGui::SliderFloat("camera rotation speed", &m_camera.m_rotationSpeed, 0.1f, 3.0f);

    for (auto& [name, renderable] : m_renderables)
    {
        if (ImGui::TreeNode(name.c_str()))
        {
            ImGui::SliderFloat3("position", &renderable.getTransformComponent()->translate.x, -25.0f, 25.0f);
            ImGui::SliderFloat("scale", &renderable.getTransformComponent()->scale.x, 0.1f, 10.0f);
            ImGui::SliderFloat3("rotation", &renderable.getTransformComponent()->rotation.x, math::XMConvertToRadians(-90.0f), math::XMConvertToRadians(90.0f));

            renderable.getTransformComponent()->scale.y = renderable.getTransformComponent()->scale.x;
            renderable.getTransformComponent()->scale.z = renderable.getTransformComponent()->scale.x;

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("light properties"))
    {
        ImGui::ColorPicker3("directional light color", &m_sceneBuffer.data.pointLightColor.x);

        ImGui::SliderFloat3("position", &m_light.getTransformComponent()->translate.x, -25.0f, 25.0f);
        ImGui::SliderFloat("scale", &m_light.getTransformComponent()->scale.x, 0.1f, 10.0f);

        m_light.getTransformComponent()->scale.y = m_light.getTransformComponent()->scale.x;
        m_light.getTransformComponent()->scale.z = m_light.getTransformComponent()->scale.x;

        ImGui::TreePop();
    }

    ImGui::End();

    auto& ctx = m_deviceContext;

    constexpr std::array<float, 4> clearColor{0.0f, 0.0f, 0.0f, 1.0f};

    ctx->ClearDepthStencilView(m_dsv.Get(), D3D11_CLEAR_DEPTH, 1.0f, 1u);

    ctx->ClearRenderTargetView(m_renderTargetView.Get(), clearColor.data());
    ctx->ClearRenderTargetView(m_offscreenRT.rtv.Get(), clearColor.data());

    // Render to offscreen RT.
    ctx->RSSetViewports(1u, &m_viewport);
    ctx->OMSetRenderTargets(1u, m_offscreenRT.rtv.GetAddressOf(), m_dsv.Get());

    bindPipeline(m_pipeline);

    ctx->VSSetConstantBuffers(0u, 1u, m_sceneBuffer.buffer.GetAddressOf());

    for (auto& [name, renderable] : m_renderables)
    {
        renderable.render(ctx.Get());
    }

    bindPipeline(m_lightPipeline);

    ctx->VSSetConstantBuffers(0u, 1u, m_sceneBuffer.buffer.GetAddressOf());
    ctx->PSSetConstantBuffers(0u, 1u, m_sceneBuffer.buffer.GetAddressOf());

    m_light.render(ctx.Get());

    // Render to swapchain backbuffer RTV.

    ctx->RSSetViewports(1u, &m_viewport);
    ctx->OMSetRenderTargets(1u, m_renderTargetView.GetAddressOf(), nullptr);

    bindPipeline(m_fullscreenPassPipeline);
    bindTexturePS(m_offscreenRT.srv.Get(), 0u);
    ctx->PSSetSamplers(0u, 1u, m_offscreenSampler.GetAddressOf());

    ctx->Draw(3u, 0u);

    wrl::ComPtr<ID3D11ShaderResourceView> nullSrv{nullptr};
    ctx->PSSetShaderResources(0u, 1u, nullSrv.GetAddressOf());

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    throwIfFailed(m_swapchain->Present(1u, 0u));
}
