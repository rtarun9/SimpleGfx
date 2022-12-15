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
        math::XMMATRIX inverseModelViewMatrix{};
    };

    struct PBRMaterial
    {
        wrl::ComPtr<ID3D11ShaderResourceView> albedoTextureSrv{};
        uint32_t albedoTextureSamplerStateIndex{};

        wrl::ComPtr<ID3D11ShaderResourceView> normalTexture{};
        uint32_t normalTextureSamplerStateIndex{};

        wrl::ComPtr<ID3D11ShaderResourceView> metalRoughnessTexture{};
        uint32_t metalRoughnessTextureSamplerStateIndex{};

        wrl::ComPtr<ID3D11ShaderResourceView> aoTexture{};
        uint32_t aoTextureSamplerStateIndex{};

        wrl::ComPtr<ID3D11ShaderResourceView> emissiveTexture{};
        uint32_t emissiveTextureSamplerStateIndex{};
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

        void updateTransformBuffer(const math::XMMATRIX viewMatrix, ID3D11DeviceContext* const deviceContext);

        void render(ID3D11DeviceContext* const deviceContext) const;

        void renderInstanced(ID3D11DeviceContext* const deviceContext, const uint32_t instanceCount) const;

      private:
        void loadSamplers(ID3D11Device* const device, tinygltf::Model* const model);
        void loadMaterials(ID3D11Device* const device, tinygltf::Model* const model);
        void loadNode(ID3D11Device* const device, uint32_t nodeIndex, tinygltf::Model* const model);

      private:
        std::vector<Mesh> m_meshes{};
        std::vector<PBRMaterial> m_materials{};
        std::vector<wrl::ComPtr<ID3D11SamplerState>> m_samplers{};

        std::string m_modelPath{};
        std::string m_modelDirectory{};

        TransformComponent m_transformComponent{};
        ConstantBuffer<TransformBuffer> m_transformBuffer{};

        wrl::ComPtr<ID3D11SamplerState> m_fallbackSamplerState{};
        wrl::ComPtr<ID3D11ShaderResourceView> m_fallbackSrv{};
    };
}
