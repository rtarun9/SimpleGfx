#include "Pch.hpp"

#include "Engine.hpp"

Engine::Engine(const std::string_view windowTitle) : sgfx::Application(windowTitle) {}

void Engine::loadContent()
{
    m_pipeline = createGraphicsPipeline(sgfx::GraphicsPipelineCreationDesc{
        .vertexShaderPath = L"shaders/TriangleShader.hlsl",
        .pixelShaderPath = L"shaders/TriangleShader.hlsl",
        .inputLayoutElements = {sgfx::InputLayoutElementDesc{.semanticName = "Position", .format = DXGI_FORMAT_R32G32B32_FLOAT, .inputClassification = D3D11_INPUT_PER_VERTEX_DATA},
                                sgfx::InputLayoutElementDesc{.semanticName = "Color", .format = DXGI_FORMAT_R32G32B32_FLOAT, .inputClassification = D3D11_INPUT_PER_VERTEX_DATA}},
        .primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
        .vertexSize = sizeof(sgfx::VertexPosColor),
    });

    constexpr std::array<sgfx::VertexPosColor, 3> vertexData = {
        sgfx::VertexPosColor{.position = {-0.5f, -0.5f, 1.0f}, .color = {1.0f, 0.0f, 0.0f}},
        sgfx::VertexPosColor{.position = {0.0f, 0.5f, 1.0f}, .color = {0.0f, 1.0f, 0.0f}},
        sgfx::VertexPosColor{.position = {0.5f, -0.5f, 1.0f}, .color = {0.0f, 0.0f, 1.0f}},
    };

    m_vertexBuffer = createBuffer<sgfx::VertexPosColor>(sgfx::BufferCreationDesc{.usage = D3D11_USAGE_IMMUTABLE, .bindFlags = D3D11_BIND_VERTEX_BUFFER}, vertexData);
}

void Engine::update() {}

void Engine::render()
{
    auto& ctx = m_deviceContext;

    constexpr std::array<float, 4> clearColor{0.2f, 0.2f, 0.2f, 1.0f};
    constexpr uint32_t offset = 0u;

    ctx->ClearRenderTargetView(m_renderTargetView.Get(), clearColor.data());
    ctx->RSSetViewports(1u, &m_viewport);

    ctx->OMSetRenderTargets(1u, m_renderTargetView.GetAddressOf(), nullptr);

    bindPipeline(m_pipeline);

    ctx->IASetVertexBuffers(0u, 1u, m_vertexBuffer.GetAddressOf(), &m_pipeline.vertexSize, &offset);

    ctx->Draw(3u, 0u);

    throwIfFailed(m_swapchain->Present(1u, 0u));
}
