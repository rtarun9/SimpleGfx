#include "Pch.hpp"

#include "Engine.hpp"

Engine::Engine(const std::string_view windowTitle) : sgfx::Application(windowTitle) {}

void Engine::loadContent()
{
    m_offscreenRT = createRenderTarget(m_windowWidth, m_windowHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);

    m_offscreenSampler = createSampler(sgfx::SamplerCreationDesc{
        .filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
        .addressMode = D3D11_TEXTURE_ADDRESS_WRAP,
    });

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

    std::jthread scifiHelmetThread([&]() { m_renderables["scifi-helmet"] = createModel("assets/models/SciFiHelmet/glTF/SciFiHelmet.gltf"); });

    m_fullscreenPassPipeline = createGraphicsPipeline(sgfx::GraphicsPipelineCreationDesc{
        .vertexShaderPath = L"shaders/FullscreenPass.hlsl",
        .pixelShaderPath = L"shaders/FullscreenPass.hlsl",
        .primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
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

    m_sceneBuffer = createConstantBuffer<sgfx::SceneBuffer>();

    m_lightModel = createModel("assets/models/Cube/glTF/Cube.gltf");

    math::XMFLOAT4 position = math::XMFLOAT4(2.2f, 2.2f, -0.5f, 1.0f);

    for (const uint32_t i : std::views::iota(0u, sgfx::LIGHT_COUNT - 1u))
    {
        m_lightPositions[i] = {position};

        position.x += 2.2f;
        position.y += 2.2f;
        position.z += -0.5f;
    }

    for (const uint32_t i : std::views::iota(0u, sgfx::LIGHT_COUNT))
    {
        m_sceneBuffer.data.lightColorIntensity[i] = {1.0f, 1.0f, 1.0f, 1.0f};
    }

    m_lightMatricesBuffer = createConstantBuffer<sgfx::LightMatrix>();

    m_dsv = createDepthStencilView();
}

void Engine::update(const float deltaTime)
{
    m_camera.update(deltaTime);

    const math::XMMATRIX viewMatrix = m_camera.getLookAtMatrix();
    const math::XMMATRIX projectionMatrix = math::XMMatrixPerspectiveFovLH(math::XMConvertToRadians(45.0f), m_windowWidth / static_cast<float>(m_windowHeight), 0.1f, 1000.0f);

    m_sceneBuffer.data.viewMatrix = viewMatrix;
    m_sceneBuffer.data.viewProjectionMatrix = viewMatrix * projectionMatrix;

    // Update scene buffer for non directional lights.
    for (const uint32_t i : std::views::iota(1u, sgfx::LIGHT_COUNT))
    {
        const math::XMVECTOR lightPosition = math::XMLoadFloat4(&m_lightPositions[i - 1u]);
        const math::XMVECTOR viewSpaceLightPosition = math::XMVector3TransformCoord(lightPosition, viewMatrix);

        math::XMStoreFloat4(&m_sceneBuffer.data.viewSpaceLightPosition[i], viewSpaceLightPosition);

        m_lightMatricesBuffer.data.lightModelMatrix[i - 1] =
            math::XMMatrixIdentity() * math::XMMatrixScaling(0.2f, 0.2f, 0.2f) * math::XMMatrixTranslationFromVector(lightPosition);
    }

    // Update directional light.
    {
        const math::XMVECTOR lightPosition = math::XMVectorSet(0.0f, sin(math::XMConvertToRadians(m_sunAngle)), cos(math::XMConvertToRadians(m_sunAngle)), 0.0f);
        const math::XMVECTOR viewSpaceLightPosition = math::XMVector4Transform(lightPosition, viewMatrix);

        math::XMStoreFloat4(&m_sceneBuffer.data.viewSpaceLightPosition[0], viewSpaceLightPosition);
    }

    updateConstantBuffer(m_sceneBuffer);
    updateConstantBuffer(m_lightMatricesBuffer);

    for (auto& [name, renderable] : m_renderables)
    {
        renderable.updateTransformBuffer(viewMatrix, m_deviceContext.Get());
    }
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
        if (ImGui::TreeNode("Directional light"))
        {
            ImGui::SliderFloat("sun Angle", &m_sunAngle, -180.0f, 180.0f);
            ImGui::SliderFloat3("dir light color", &m_sceneBuffer.data.lightColorIntensity[0].x, 0.0f, 1.0f);
            ImGui::SliderFloat("dir light intensity", &m_sceneBuffer.data.lightColorIntensity[0].w, 0.0f, 30.0f);

            ImGui::TreePop();
        }

        for (const uint32_t i : std::views::iota(1u, sgfx::LIGHT_COUNT))
        {
            if (ImGui::TreeNode((std::string("Point Light") + std::to_string(i)).c_str()))
            {
                ImGui::ColorPicker3("light color", &m_sceneBuffer.data.lightColorIntensity[i].x);
                ImGui::SliderFloat("Intensity", &m_sceneBuffer.data.lightColorIntensity[i].w, 0.1f, 30.0f);

                ImGui::SliderFloat3("position", &m_lightPositions[i - 1].x, -25.0f, 25.0f);

                ImGui::TreePop();
            }
        }

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
    ctx->PSSetConstantBuffers(0u, 1u, m_sceneBuffer.buffer.GetAddressOf());

    for (auto& [name, renderable] : m_renderables)
    {
        renderable.render(ctx.Get());
    }

    bindPipeline(m_lightPipeline);

    ctx->VSSetConstantBuffers(0u, 1u, m_sceneBuffer.buffer.GetAddressOf());

    ctx->VSSetConstantBuffers(2u, 1u, m_lightMatricesBuffer.buffer.GetAddressOf());
    m_lightModel.renderInstanced(ctx.Get(), sgfx::LIGHT_COUNT - 1u);

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
