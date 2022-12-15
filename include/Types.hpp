#pragma once

namespace sgfx
{
    static constexpr uint32_t INVALID_INDEX_U32 = -1;

    struct VertexPosColor
    {
        math::XMFLOAT3 position{};
        math::XMFLOAT3 color{};
    };

    struct VertexPosColorTexCoord
    {
        math::XMFLOAT3 position{};
        math::XMFLOAT3 color{};
        math::XMFLOAT2 texCoord{};
    };

    struct ModelVertex
    {
        math::XMFLOAT3 position{};
        math::XMFLOAT2 textureCoord{};
        math::XMFLOAT3 normal{};
    };

    static constexpr uint32_t LIGHT_COUNT = 5u;

    struct alignas(256) SceneBuffer
    {
        math::XMMATRIX viewMatrix{};
        math::XMMATRIX viewProjectionMatrix{};

        math::XMFLOAT4 lightColorIntensity[LIGHT_COUNT]{};
        math::XMFLOAT4 viewSpaceLightPosition[LIGHT_COUNT]{};
    };

    struct alignas(256) LightMatrix
    {
        math::XMMATRIX lightModelMatrix[sgfx::LIGHT_COUNT - 1u];
    };

    struct InputLayoutElementDesc
    {
        std::string semanticName{};
        DXGI_FORMAT format{};
        D3D11_INPUT_CLASSIFICATION inputClassification{};
    };

    struct SamplerCreationDesc
    {
        D3D11_FILTER filter{};
        D3D11_TEXTURE_ADDRESS_MODE addressMode{};
    };

    struct BufferCreationDesc
    {
        D3D11_USAGE usage;
        uint32_t bindFlags{};
    };

    template <typename T> struct ConstantBuffer
    {
        wrl::ComPtr<ID3D11Buffer> buffer{};
        T data{};
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
        wrl::ComPtr<ID3D11VertexShader> vertexShader{};
        wrl::ComPtr<ID3D11PixelShader> pixelShader{};
        wrl::ComPtr<ID3D11InputLayout> inputLayout{};

        D3D11_PRIMITIVE_TOPOLOGY primitiveTopology{};

        uint32_t vertexSize{};
    };

    struct RenderTarget
    {
        wrl::ComPtr<ID3D11Texture2D> texture{};

        wrl::ComPtr<ID3D11RenderTargetView> rtv{};
        wrl::ComPtr<ID3D11ShaderResourceView> srv{};
    };
}