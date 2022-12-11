#pragma once

namespace sgfx
{
    struct VertexPosColor
    {
        math::XMFLOAT3 position{};
        math::XMFLOAT3 color{};
    };

    struct InputLayoutElementDesc
    {
        std::string semanticName{};
        DXGI_FORMAT format{};
        D3D11_INPUT_CLASSIFICATION inputClassification{};
    };

    struct BufferCreationDesc
    {
        D3D11_USAGE usage;
        uint32_t bindFlags{};
    };

    struct GraphicsPipelineCreationDesc
    {
        std::wstring vertexShaderPath{};
        std::wstring pixelShaderPath{};

        std::vector<InputLayoutElementDesc> inputLayoutElements{};

        D3D11_PRIMITIVE_TOPOLOGY primitiveTopology{};
        uint32_t vertexSize{};
    };

    struct GraphicsPipeline
    {
        Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader{};
        Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader{};
        Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout{};

        D3D11_PRIMITIVE_TOPOLOGY primitiveTopology{};

        uint32_t vertexSize{};
    };
}