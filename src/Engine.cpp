#include "Pch.hpp"

#include "Engine.hpp"

using namespace math;

Engine::Engine(const std::string_view windowTitle) : sgfx::Application(windowTitle) {}

void Engine::loadContent()
{
    m_offscreenRT = createRenderTarget(m_windowWidth, m_windowHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);

    m_offscreenSampler = createSampler(sgfx::SamplerCreationDesc{
        .filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
        .addressMode = D3D11_TEXTURE_ADDRESS_CLAMP,
    });

    m_wrapSampler = createSampler(sgfx::SamplerCreationDesc{
        .filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        .addressMode = D3D11_TEXTURE_ADDRESS_WRAP,
    });

    m_ssaoRt = createRenderTarget(m_windowWidth, m_windowHeight, DXGI_FORMAT_R8_UNORM);
    m_ssaoBlurredRt = createRenderTarget(m_windowWidth, m_windowHeight, DXGI_FORMAT_R8_UNORM);

    m_renderables["cube"] = createModel("assets/models/Cube/glTF/Cube.gltf");

    m_renderables["cube2"] = createModel("assets/models/Cube/glTF/Cube.gltf", sgfx::TransformComponent{.translate = {5.0f, 0.0f, -2.0f}});

    m_renderables["sponza"] = createModel("assets/models/sponza-gltf-pbr/sponza.glb", sgfx::TransformComponent{.scale = {0.1f, 0.1f, 0.1f}});

    m_renderables["scifi-helmet"] = createModel("assets/models/SciFiHelmet/glTF/SciFiHelmet.gltf");

    m_fullscreenPassPipeline = createGraphicsPipeline(sgfx::GraphicsPipelineCreationDesc{
        .vertexShaderPath = L"shaders/FullscreenPass.hlsl",
        .pixelShaderPath = L"shaders/FullscreenPass.hlsl",
        .primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    });

    m_pipeline = createGraphicsPipeline(sgfx::GraphicsPipelineCreationDesc{
        .vertexShaderPath = L"shaders/PhongShader.hlsl",
        .pixelShaderPath = L"shaders/PhongShader.hlsl",
        .primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    });

    m_ssaoPipeline = createGraphicsPipeline(sgfx::GraphicsPipelineCreationDesc{
        .vertexShaderPath = L"shaders/SSAO.hlsl",
        .pixelShaderPath = L"shaders/SSAO.hlsl",
        .primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,

    });

    m_boxBlurPipeline = createGraphicsPipeline(sgfx::GraphicsPipelineCreationDesc{
        .vertexShaderPath = L"shaders/BoxBlur.hlsl",
        .pixelShaderPath = L"shaders/BoxBlur.hlsl",
        .primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,

    });

    m_lightPipeline = createGraphicsPipeline(sgfx::GraphicsPipelineCreationDesc{
        .vertexShaderPath = L"shaders/LightShader.hlsl",
        .pixelShaderPath = L"shaders/LightShader.hlsl",
        .inputLayoutElements =
            {
                sgfx::InputLayoutElementDesc{.semanticName = "Position", .format = DXGI_FORMAT_R32G32B32_FLOAT, .inputClassification = D3D11_INPUT_PER_VERTEX_DATA},
                sgfx::InputLayoutElementDesc{.semanticName = "TextureCoord", .format = DXGI_FORMAT_R32G32_FLOAT, .inputClassification = D3D11_INPUT_PER_VERTEX_DATA},
                sgfx::InputLayoutElementDesc{.semanticName = "Normal", .format = DXGI_FORMAT_R32G32B32_FLOAT, .inputClassification = D3D11_INPUT_PER_VERTEX_DATA},
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

    m_gpassDepthTexture = createDepthTexture();

    m_gpassRts[0] = createRenderTarget(m_windowWidth, m_windowHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
    m_gpassRts[1] = createRenderTarget(m_windowWidth, m_windowHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);
    m_gpassRts[2] = createRenderTarget(m_windowWidth, m_windowHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);

    m_gpassPipeline = createGraphicsPipeline(sgfx::GraphicsPipelineCreationDesc{
        .vertexShaderPath = L"shaders/GPass.hlsl",
        .pixelShaderPath = L"shaders/GPass.hlsl",
        .inputLayoutElements =
            {
                sgfx::InputLayoutElementDesc{.semanticName = "Position", .format = DXGI_FORMAT_R32G32B32_FLOAT, .inputClassification = D3D11_INPUT_PER_VERTEX_DATA},
                sgfx::InputLayoutElementDesc{.semanticName = "TextureCoord", .format = DXGI_FORMAT_R32G32_FLOAT, .inputClassification = D3D11_INPUT_PER_VERTEX_DATA},
                sgfx::InputLayoutElementDesc{.semanticName = "Normal", .format = DXGI_FORMAT_R32G32B32_FLOAT, .inputClassification = D3D11_INPUT_PER_VERTEX_DATA},
            },
        .primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
        .vertexSize = sizeof(sgfx::ModelVertex),
    });

    m_gpassDepthTexture = createDepthTexture();

    m_depthTexture = createDepthTexture();

    // Setup data from SSAO pass.
    m_ssaoBuffer = createConstantBuffer<sgfx::SSAOBuffer>();

    std::uniform_real_distribution<float> randomUnitFloatDistribution{0.0f, 1.0f};
    std::default_random_engine generator{};

    // Randomly generator kernel vectors in Tangent space (i.e the vector points in the +ve Z direction).
    // X and Y components are in range [-1, 1], Z component is in range [0, 1].
    for (const uint32_t i : std::views::iota(0u, 64u))
    {
        const math::XMVECTOR randomVector = math::XMVectorSet(randomUnitFloatDistribution(generator) * 2.0f - 1.0f,
                                                              randomUnitFloatDistribution(generator) * 2.0f - 1.0f,
                                                              randomUnitFloatDistribution(generator),
                                                              0.0f);

        // After normalization, the vector has unit length, i.e it lies on the surface of the hemisphere oriented along the +z axis.
        const math::XMVECTOR normalizedRandomVector = math::XMVector3Normalize(randomVector);

        // Scaling the sample point so the points lie within the hemisphere.
        const math::XMVECTOR randomlyDistributedSamplePoint = randomUnitFloatDistribution(generator) * normalizedRandomVector;

        // We want more sample points generated near the origin and fewer points as the distance from origin increases.
        // This is done because we want to give more weight / priority to occlusions happening close to the actual fragment.

        const float scaleFactor = static_cast<float>(i) / 64.0f;
        const float scale = std::lerp<float, float>(0.1f, 1.0f, scaleFactor * scaleFactor);

        math::XMStoreFloat4(&m_ssaoBuffer.data.sampleVectors[i], scale * randomlyDistributedSamplePoint);
    }

    // Generate the noise kernel.
    // Contents are used to randomly rotate the kernel vectors.
    // Z component not taken into account as it will always be 0 (i.e we want rotation around the z axis).
    std::array<math::XMFLOAT2, 64> noiseTextureData{};
    for (const uint32_t i : std::views::iota(0u, 64u))
    {
        noiseTextureData[i] = math::XMFLOAT2{randomUnitFloatDistribution(generator) * 2.0f - 1.0f, randomUnitFloatDistribution(generator) * 2.0f - 1.0f};
    }

    m_ssaoRandomRotationTexture = createTexture<math::XMFLOAT2>(noiseTextureData, 8u, 8u, DXGI_FORMAT_R32G32_FLOAT);
}

void Engine::update(const float deltaTime)
{
    m_camera.update(deltaTime);

    const math::XMMATRIX viewMatrix = m_camera.getLookAtMatrix();
    const math::XMMATRIX projectionMatrix = math::XMMatrixPerspectiveFovLH(math::XMConvertToRadians(45.0f), m_windowWidth / static_cast<float>(m_windowHeight), 0.1f, 230.0f);

    m_sceneBuffer.data.viewMatrix = viewMatrix;
    m_sceneBuffer.data.viewProjectionMatrix = viewMatrix * projectionMatrix;

    m_ssaoBuffer.data.projectionMatrix = projectionMatrix;

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
    updateConstantBuffer(m_ssaoBuffer);

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

    ImGui::SliderFloat("ssao radius", &m_ssaoBuffer.data.radius, 0.0f, 10.0f);
    ImGui::SliderFloat("ssao bias", &m_ssaoBuffer.data.bias, 0.0f, 10.0f);
    ImGui::SliderFloat("ssao power", &m_ssaoBuffer.data.power, 0.0f, 10.0f);

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

    ImGui::Begin("SSAO RT");
    ImGui::Image(m_ssaoRt.srv.Get(), {300, 300});
    ImGui::End();

    ImGui::Begin("SSAO Blurred RT");
    ImGui::Image(m_ssaoBlurredRt.srv.Get(), {300, 300});
    ImGui::End();

    auto& ctx = m_deviceContext;

    constexpr std::array<float, 4> clearColor{0.0f, 0.0f, 0.0f, 1.0f};

    ctx->ClearDepthStencilView(m_depthTexture.dsv.Get(), D3D11_CLEAR_DEPTH, 1.0f, 1u);
    ctx->ClearDepthStencilView(m_gpassDepthTexture.dsv.Get(), D3D11_CLEAR_DEPTH, 1.0f, 1u);

    ctx->ClearRenderTargetView(m_renderTargetView.Get(), clearColor.data());
    ctx->ClearRenderTargetView(m_renderTargetView.Get(), clearColor.data());
    ctx->ClearRenderTargetView(m_ssaoRt.rtv.Get(), clearColor.data());

    std::array<ID3D11ShaderResourceView*, 4> nullSrvs{nullptr, nullptr, nullptr, nullptr};

    for (auto& renderTarget : m_gpassRts)
    {
        ctx->ClearRenderTargetView(renderTarget.rtv.Get(), clearColor.data());
    }

    // Render the Geometry pass.
    const std::array<ID3D11RenderTargetView*, 3u> gpassRtvs{m_gpassRts[0].rtv.Get(), m_gpassRts[1].rtv.Get(), m_gpassRts[2].rtv.Get()};
    ctx->OMSetRenderTargets(3u, gpassRtvs.data(), m_gpassDepthTexture.dsv.Get());
    ctx->RSSetViewports(1u, &m_viewport);

    bindPipeline(m_gpassPipeline);
    ctx->VSSetConstantBuffers(0u, 1u, m_sceneBuffer.buffer.GetAddressOf());
    ctx->PSSetConstantBuffers(0u, 1u, m_sceneBuffer.buffer.GetAddressOf());

    for (auto& [name, renderable] : m_renderables)
    {
        renderable.render(ctx.Get());
    }

    // SSAO pass.
    ctx->OMSetRenderTargets(1u, m_ssaoRt.rtv.GetAddressOf(), nullptr);
    bindPipeline(m_ssaoPipeline);

    ctx->VSSetConstantBuffers(0u, 1u, m_sceneBuffer.buffer.GetAddressOf());
    ctx->PSSetConstantBuffers(0u, 1u, m_sceneBuffer.buffer.GetAddressOf());
    ctx->PSSetConstantBuffers(1u, 1u, m_ssaoBuffer.buffer.GetAddressOf());
    ctx->PSSetSamplers(0u, 1u, m_offscreenSampler.GetAddressOf());
    ctx->PSSetSamplers(1u, 1u, m_wrapSampler.GetAddressOf());

    const std::array<ID3D11ShaderResourceView* const, 3u> ssaoShaderTextures{m_ssaoRandomRotationTexture.Get(), m_gpassRts[1].srv.Get(), m_gpassRts[2].srv.Get()};

    ctx->PSSetShaderResources(0u, 3u, ssaoShaderTextures.data());

    ctx->Draw(3u, 0u);

    // Apply box blur filter on SSAO texture.

    ctx->OMSetRenderTargets(1u, m_ssaoBlurredRt.rtv.GetAddressOf(), nullptr);
    bindPipeline(m_boxBlurPipeline);

    ctx->PSSetShaderResources(0u, 1u, nullSrvs.data());

    ctx->PSSetShaderResources(0u, 1u, m_ssaoRt.srv.GetAddressOf());
    ctx->Draw(3u, 0u);

    // Shading pass.
    ctx->OMSetRenderTargets(1u, m_offscreenRT.rtv.GetAddressOf(), nullptr);

    bindPipeline(m_pipeline);

    const std::array<ID3D11ShaderResourceView*, 4u> lightingPassSrvs{m_gpassRts[0].srv.Get(), m_gpassRts[1].srv.Get(), m_gpassRts[2].srv.Get(), m_ssaoBlurredRt.srv.Get()};

    ctx->VSSetConstantBuffers(0u, 1u, m_sceneBuffer.buffer.GetAddressOf());
    ctx->PSSetConstantBuffers(0u, 1u, m_sceneBuffer.buffer.GetAddressOf());

    ctx->PSSetShaderResources(0u, 4u, lightingPassSrvs.data());
    ctx->Draw(3u, 0u);

    ctx->OMSetRenderTargets(1u, m_offscreenRT.rtv.GetAddressOf(), m_gpassDepthTexture.dsv.Get());
    ctx->RSSetViewports(1u, &m_viewport);

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

    ctx->PSSetShaderResources(0u, 4u, nullSrvs.data());

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    throwIfFailed(m_swapchain->Present(1u, 0u));
}
