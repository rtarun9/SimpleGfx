#pragma once

namespace tinygltf
{
    class Model;
}

namespace sgfx
{
    struct TransformComponent
    {
        math::XMFLOAT3 rotation{0.0f, 0.0f, 0.0f};
        math::XMFLOAT3 scale{1.0f, 1.0f, 1.0f};
        math::XMFLOAT3 translate{0.0f, 0.0f, 0.0f};
    };

    struct alignas(256) TransformBuffer
    {
        math::XMMATRIX modelMatrix{};
        math::XMMATRIX inverseModelMatrix{};
    };

    struct PBRMaterial
    {
        wrl::ComPtr<ID3D11ShaderResourceView> albedoTextureSrv{};
        wrl::ComPtr<ID3D11SamplerState> albedoTextureSamplerState{};

        wrl::ComPtr<ID3D11ShaderResourceView> normalTexture{};
        wrl::ComPtr<ID3D11SamplerState> normalTextureSamplerState{};

        wrl::ComPtr<ID3D11ShaderResourceView> metalRoughnessTexture{};
        wrl::ComPtr<ID3D11SamplerState> metalRoughnessTextureSamplerState{};

        wrl::ComPtr<ID3D11ShaderResourceView> aoTexture{};
        wrl::ComPtr<ID3D11SamplerState> aoTextureSamplerState{};

        wrl::ComPtr<ID3D11ShaderResourceView> emissiveTexture{};
        wrl::ComPtr<ID3D11SamplerState> emissiveTextureSamplerState{};
    };

    struct Mesh
    {
        wrl::ComPtr<ID3D11Buffer> vertexBuffer{};
        wrl::ComPtr<ID3D11Buffer> indexBuffer{};
        uint32_t indicesCount{};

        uint32_t materialIndex{};
    };

    class Model
    {
      public:
        Model() = default;
        Model(ID3D11Device* const device, ID3D11ShaderResourceView* const fallbackSrv, const std::string_view modelPath);

        TransformComponent* getTransformComponent() { return &m_transformComponent; }

        void updateTransformBuffer(ID3D11DeviceContext* const deviceContext);

        void render(ID3D11DeviceContext* const deviceContext) const;

      private:
        [[nodiscard]] std::vector<wrl::ComPtr<ID3D11SamplerState>> loadSamplers(ID3D11Device* const device, tinygltf::Model* const model);
        void loadMaterials(ID3D11Device* const device, const std::span<const wrl::ComPtr<ID3D11SamplerState>> samplers, tinygltf::Model* const model);
        void loadNode(ID3D11Device* const device, uint32_t nodeIndex, tinygltf::Model* const model);

      private:
        std::vector<Mesh> m_meshes{};
        std::vector<PBRMaterial> m_materials{};

        std::string m_modelPath{};
        std::string m_modelDirectory{};

        TransformComponent m_transformComponent{};
        ConstantBuffer<TransformBuffer> m_transformBuffer{};

        wrl::ComPtr<ID3D11SamplerState> m_fallbackSamplerState{};
        wrl::ComPtr<ID3D11ShaderResourceView> m_fallbackSrv{};
    };
}
