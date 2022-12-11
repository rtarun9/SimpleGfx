#include "Pch.hpp"

#include "Engine.hpp"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_sdl.h>

Engine::Engine(const std::string_view windowTitle) : sgfx::Application(windowTitle) {}

void Engine::loadContent()
{
    m_pipeline = createGraphicsPipeline(sgfx::GraphicsPipelineCreationDesc{
        .vertexShaderPath = L"shaders/TestShader.hlsl",
        .pixelShaderPath = L"shaders/TestShader.hlsl",
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

    m_sceneBuffer.buffer = createBuffer<sgfx::TransformBuffer>(sgfx::BufferCreationDesc{.usage = D3D11_USAGE_DEFAULT, .bindFlags = D3D11_BIND_CONSTANT_BUFFER});

    m_cube = createModel("assets/models/Cube/glTF/Cube.gltf");

    m_dsv = createDepthStencilView();
}

void Engine::update(const float deltaTime)
{
    m_camera.update(deltaTime);

    const math::XMMATRIX viewMatrix = m_camera.getLookAtMatrix();
    const math::XMMATRIX projectionMatrix = math::XMMatrixPerspectiveFovLH(math::XMConvertToRadians(45.0f), m_windowWidth / static_cast<float>(m_windowHeight), 0.1f, 1000.0f);

    m_sceneBuffer.data.viewProjectionMatrix = viewMatrix * projectionMatrix;

    updateConstantBuffer(m_sceneBuffer);

    m_cube.updateTransformBuffer(m_deviceContext.Get());
}

void Engine::render()
{
    // Start the Dear ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Scene menu");
    ImGui::SliderFloat("camera mvmt speed", &m_camera.m_movementSpeed, 0.1f, 50.0f);
    ImGui::End();
    
    auto& ctx = m_deviceContext;

    constexpr std::array<float, 4> clearColor{0.2f, 0.2f, 0.2f, 1.0f};
    constexpr uint32_t offset = 0u;

    ctx->ClearDepthStencilView(m_dsv.Get(), D3D11_CLEAR_DEPTH, 1.0f, 1u);

    ctx->ClearRenderTargetView(m_renderTargetView.Get(), clearColor.data());
    ctx->RSSetViewports(1u, &m_viewport);

    ctx->OMSetRenderTargets(1u, m_renderTargetView.GetAddressOf(), m_dsv.Get());

    bindPipeline(m_pipeline);

    ctx->VSSetConstantBuffers(0u, 1u, m_sceneBuffer.buffer.GetAddressOf());

    m_cube.render(m_deviceContext.Get());

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    throwIfFailed(m_swapchain->Present(1u, 0u));
}
