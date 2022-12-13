#include "Pch.hpp"

#include "Model.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#include <tiny_gltf.h>

#include <DirectXTex.h>

namespace sgfx
{
    Model::Model(ID3D11Device* const device, ID3D11ShaderResourceView* const fallbackSrv, const std::string_view modelPath) : m_modelPath(modelPath), m_fallbackSrv(fallbackSrv)
    {
        // Create the transform buffer.
        const D3D11_BUFFER_DESC bufferDesc = {
            .ByteWidth = static_cast<uint32_t>(sizeof(TransformBuffer)),
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
        };

        throwIfFailed(device->CreateBuffer(&bufferDesc, nullptr, &m_transformBuffer.buffer));

        std::string modelDirectoryPathStr{};

        if (m_modelPath.find_last_of("/\\") != std::string::npos)
        {
            m_modelDirectory = m_modelPath.substr(0, modelPath.find_last_of("/\\")) + "/";
        }

        std::string warning{};
        std::string error{};

        tinygltf::TinyGLTF context{};

        tinygltf::Model model{};

        if (m_modelPath.find(".glb") != std::string::npos)
        {
            if (!context.LoadBinaryFromFile(&model, &error, &warning, m_modelPath))
            {
                if (!error.empty())
                {
                    fatalError(error);
                }

                if (!warning.empty())
                {
                    fatalError(warning);
                }
            }
        }
        else
        {
            if (!context.LoadASCIIFromFile(&model, &error, &warning, m_modelPath))
            {
                if (!error.empty())
                {
                    fatalError(error);
                }

                if (!warning.empty())
                {
                    fatalError(warning);
                }
            }
        }

        std::thread samplerThread([&]() { // Load samplers.
            loadSamplers(device, &model);
        });

        std::thread materialThread( // Load materials.
            [&]()
            {
                // Load textures and materials.
                loadMaterials(device, &model);
            });

        tinygltf::Scene& scene = model.scenes[model.defaultScene];

        std::thread meshThread([&]() { // Build meshes.
            for (const int& nodeIndex : scene.nodes)
            {
                loadNode(device, nodeIndex, &model);
            }
        });

        samplerThread.join();
        materialThread.join();
        meshThread.join();
    }

    void Model::updateTransformBuffer(const math::XMMATRIX viewMatrix, ID3D11DeviceContext* const deviceContext)
    {
        const DirectX::XMVECTOR scalingVector = DirectX::XMLoadFloat3(&m_transformComponent.scale);
        const DirectX::XMVECTOR rotationVector = DirectX::XMLoadFloat3(&m_transformComponent.rotation);
        const DirectX::XMVECTOR translationVector = DirectX::XMLoadFloat3(&m_transformComponent.translate);

        const DirectX::XMMATRIX modelMatrix = DirectX::XMMatrixScalingFromVector(scalingVector) * DirectX::XMMatrixRotationRollPitchYawFromVector(rotationVector) *
                                              DirectX::XMMatrixTranslationFromVector(translationVector);
        m_transformBuffer.data = {
            .modelMatrix = modelMatrix,
            .inverseModelMatrix = DirectX::XMMatrixInverse(nullptr, modelMatrix),
            .inverseModelViewMatrix = DirectX::XMMatrixInverse(nullptr, modelMatrix * viewMatrix),
        };

        deviceContext->UpdateSubresource(m_transformBuffer.buffer.Get(), 0u, nullptr, &m_transformBuffer.data, 0u, 0u);
    }

    void Model::render(ID3D11DeviceContext* const deviceContext) const
    {
        deviceContext->VSSetConstantBuffers(1u, 1u, m_transformBuffer.buffer.GetAddressOf());
        uint32_t meshIndex = 0u;

        for (const auto& mesh : m_meshes)
        {
            constexpr uint32_t stride = sizeof(ModelVertex);
            constexpr uint32_t offset = 0u;

            deviceContext->IASetVertexBuffers(0u, 1u, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
            deviceContext->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0u);

            const auto albedoTexturesampler = m_materials[mesh.materialIndex].albedoTextureSamplerStateIndex == INVALID_INDEX_U32
                                                  ? m_fallbackSamplerState.GetAddressOf()
                                                  : m_samplers[m_materials[mesh.materialIndex].albedoTextureSamplerStateIndex].GetAddressOf();

            const auto albedoSrv = m_materials[mesh.materialIndex].albedoTextureSrv.GetAddressOf();

            const auto normalTextureSampler = m_materials[mesh.materialIndex].normalTextureSamplerStateIndex == INVALID_INDEX_U32
                                                  ? m_fallbackSamplerState.GetAddressOf()
                                                  : m_samplers[m_materials[mesh.materialIndex].normalTextureSamplerStateIndex].GetAddressOf();

            const auto normalSrv = m_materials[mesh.materialIndex].normalTexture.GetAddressOf();

            // Albedo texture and sampler.
            deviceContext->PSSetSamplers(0u, 1u, albedoTexturesampler);
            deviceContext->PSSetShaderResources(0u, 1u, albedoSrv);

            // Normal texture and sampler.
            deviceContext->PSSetSamplers(1u, 1u, normalTextureSampler);
            deviceContext->PSSetShaderResources(1u, 1u, normalSrv);

            deviceContext->DrawIndexed(mesh.indicesCount, 0u, 0u);

            meshIndex++;
        }
    }

    void Model::loadSamplers(ID3D11Device* const device, tinygltf::Model* const model)
    {
        // Create fallback sampler.
        const D3D11_SAMPLER_DESC samplerDesc = {
            .Filter = D3D11_FILTER_ANISOTROPIC,
            .AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
            .AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
            .AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
            .MaxAnisotropy = D3D11_MAX_MAXANISOTROPY,
            .ComparisonFunc = D3D11_COMPARISON_NEVER,
        };

        throwIfFailed(device->CreateSamplerState(&samplerDesc, &m_fallbackSamplerState));

        m_samplers.resize(model->samplers.size());

        size_t index{0};

        for (tinygltf::Sampler& sampler : model->samplers)
        {
            D3D11_SAMPLER_DESC samplerDesc{};

            switch (sampler.minFilter)
            {
                case TINYGLTF_TEXTURE_FILTER_NEAREST:
                    {
                        if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
                        {
                            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
                        }
                        else
                        {
                            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
                        }
                    }
                    break;

                case TINYGLTF_TEXTURE_FILTER_LINEAR:
                    {
                        if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
                        {
                            samplerDesc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
                        }
                        else
                        {
                            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
                        }
                    }
                    break;

                case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
                    {
                        if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
                        {
                            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
                        }
                        else
                        {
                            samplerDesc.Filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
                        }
                    }
                    break;

                case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
                    {
                        if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
                        {
                            samplerDesc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
                        }
                        else
                        {
                            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
                        }
                    }
                    break;

                case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
                    {
                        if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
                        {
                            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
                        }
                        else
                        {
                            samplerDesc.Filter = D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
                        }
                    }
                    break;

                case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
                    {
                        if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
                        {
                            samplerDesc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
                        }
                        else
                        {
                            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
                        }
                    }
                    break;

                default:
                    {
                        samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
                    }
                    break;
            }

            auto toTextureAddressMode = [](int wrap)
            {
                switch (wrap)
                {
                    case TINYGLTF_TEXTURE_WRAP_REPEAT:
                        {
                            return D3D11_TEXTURE_ADDRESS_WRAP;
                        }
                        break;

                    case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
                        {
                            return D3D11_TEXTURE_ADDRESS_CLAMP;
                        }
                        break;

                    case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
                        {
                            return D3D11_TEXTURE_ADDRESS_MIRROR;
                        }
                        break;

                    default:
                        {
                            return D3D11_TEXTURE_ADDRESS_WRAP;
                        }
                        break;
                }
            };

            samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
            samplerDesc.AddressU = toTextureAddressMode(sampler.wrapS);
            samplerDesc.AddressV = toTextureAddressMode(sampler.wrapT);
            samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
            samplerDesc.MinLOD = 0.0f;
            samplerDesc.MipLODBias = 0.0f;
            samplerDesc.MaxAnisotropy = 16;
            samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;

            wrl::ComPtr<ID3D11SamplerState> samplerState{};
            throwIfFailed(device->CreateSamplerState(&samplerDesc, &samplerState));
            m_samplers[index++] = std::move(samplerState);
        }
    }

    void Model::loadMaterials(ID3D11Device* const device, tinygltf::Model* const model)
    {
        std::mutex textureCreationMutex{};

        auto createTexture = [&](tinygltf::Image& image, bool isSrgb)
        {
            const std::wstring texturePath = stringToWString(m_modelDirectory + image.uri);

            wrl::ComPtr<ID3D11ShaderResourceView> srv{};

            DirectX::TexMetadata metaData{};
            DirectX::ScratchImage scratchImage{};

            const DirectX::WIC_FLAGS wicFlag = isSrgb == true ? DirectX::WIC_FLAGS_DEFAULT_SRGB : DirectX::WIC_FLAGS_IGNORE_SRGB;

            const std::lock_guard<std::mutex> lock(textureCreationMutex);

            if (FAILED(DirectX::LoadFromWICFile(texturePath.data(), wicFlag, &metaData, scratchImage)))
            {
                std::wcout << L"Failed to load texture from path : " << texturePath << L'\n';
                throw std::runtime_error("Texture Loading Error");
            }

            DirectX::ScratchImage mipChain{};
            throwIfFailed(DirectX::GenerateMipMaps(scratchImage.GetImages(), scratchImage.GetImageCount(), metaData, DirectX::TEX_FILTER_DEFAULT, 0u, mipChain));

            wrl::ComPtr<ID3D11Resource> texture{};
            throwIfFailed(DirectX::CreateTexture(device, mipChain.GetImages(), mipChain.GetImageCount(), mipChain.GetMetadata(), &texture));

            throwIfFailed(DirectX::CreateShaderResourceView(device, mipChain.GetImages(), mipChain.GetImageCount(), mipChain.GetMetadata(), &srv));

            return srv;
        };

        size_t index{0};
        m_materials.resize(model->materials.size());

        for (const tinygltf::Material& material : model->materials)
        {
            PBRMaterial pbrMaterial{};

            std::jthread albedoThread(
                [&]()
                {
                    if (material.pbrMetallicRoughness.baseColorTexture.index >= 0)
                    {

                        tinygltf::Texture& albedoTexture = model->textures[material.pbrMetallicRoughness.baseColorTexture.index];
                        tinygltf::Image& albedoImage = model->images[albedoTexture.source];

                        pbrMaterial.albedoTextureSrv = createTexture(albedoImage, true);
                        pbrMaterial.albedoTextureSamplerStateIndex = albedoTexture.sampler;
                    }
                    else
                    {
                        pbrMaterial.albedoTextureSrv = m_fallbackSrv;
                        pbrMaterial.albedoTextureSamplerStateIndex = INVALID_INDEX_U32;
                    }
                });

            std::jthread metalRoughnessThread(
                [&]()
                {
                    if (material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
                    {

                        tinygltf::Texture& metalRoughnessTexture = model->textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index];
                        tinygltf::Image& metalRoughnessImage = model->images[metalRoughnessTexture.source];

                        pbrMaterial.metalRoughnessTexture = createTexture(metalRoughnessImage, false);
                        pbrMaterial.metalRoughnessTextureSamplerStateIndex = metalRoughnessTexture.sampler;
                    }
                });

            std::jthread normalThread(
                [&]()
                {
                    if (material.normalTexture.index >= 0)
                    {
                        tinygltf::Texture& normalTexture = model->textures[material.normalTexture.index];
                        tinygltf::Image& normalImage = model->images[normalTexture.source];

                        pbrMaterial.normalTexture = createTexture(normalImage, false);
                        pbrMaterial.normalTextureSamplerStateIndex = normalTexture.sampler;
                    }
                    else
                    {
                        pbrMaterial.normalTextureSamplerStateIndex = INVALID_INDEX_U32;
                    }
                });

            std::jthread occlusionThread(
                [&]()
                {
                    if (material.occlusionTexture.index >= 0)
                    {
                        tinygltf::Texture& aoTexture = model->textures[material.occlusionTexture.index];
                        tinygltf::Image& aoImage = model->images[aoTexture.source];

                        pbrMaterial.aoTexture = createTexture(aoImage, false);
                        pbrMaterial.aoTextureSamplerStateIndex = aoTexture.sampler;
                    }
                });

            std::jthread emissiveThread(
                [&]()
                {
                    if (material.emissiveTexture.index >= 0)
                    {
                        tinygltf::Texture& emissiveTexture = model->textures[material.emissiveTexture.index];
                        tinygltf::Image& emissiveImage = model->images[emissiveTexture.source];

                        pbrMaterial.emissiveTexture = createTexture(emissiveImage, true);
                        pbrMaterial.emissiveTextureSamplerStateIndex = emissiveTexture.sampler;
                    }
                });

            albedoThread.join();
            metalRoughnessThread.join();
            normalThread.join();
            occlusionThread.join();
            emissiveThread.join();

            m_materials[index++] = std::move(pbrMaterial);
        }
    }
    void Model::loadNode(ID3D11Device* const device, uint32_t nodeIndex, tinygltf::Model* const model)
    {
        const tinygltf::Node& node = model->nodes[nodeIndex];
        if (node.mesh < 0)
        {
            // Load children immediatly, as it may have some.
            for (const int& childrenNodeIndex : node.children)
            {
                loadNode(device, childrenNodeIndex, model);
            }

            return;
        }

        tinygltf::Mesh& nodeMesh = model->meshes[node.mesh];
        for (size_t i = 0; i < nodeMesh.primitives.size(); ++i)
        {
            Mesh mesh{};

            struct ModelVertex
            {
                math::XMFLOAT3 position{};
                math::XMFLOAT2 textureCoord{};
                math::XMFLOAT3 normal{};
                math::XMFLOAT4 tangent{};
                math::XMFLOAT3 biTangent{};
            };

            std::vector<ModelVertex> vertices{};

            std::vector<uint32_t> indices{};

            // Reference used : https://github.com/mateeeeeee/Adria-DX12/blob/fc98468095bf5688a186ca84d94990ccd2f459b0/Adria/Rendering/EntityLoader.cpp.

            // Get Accesor, buffer view and buffer for each attribute (position, textureCoord, normal).
            tinygltf::Primitive primitive = nodeMesh.primitives[i];
            const tinygltf::Accessor& indexAccesor = model->accessors[primitive.indices];

            // Position data.
            const tinygltf::Accessor& positionAccesor = model->accessors[primitive.attributes["POSITION"]];
            const tinygltf::BufferView& positionBufferView = model->bufferViews[positionAccesor.bufferView];
            const tinygltf::Buffer& positionBuffer = model->buffers[positionBufferView.buffer];

            const int positionByteStride = positionAccesor.ByteStride(positionBufferView);
            uint8_t const* const positions = &positionBuffer.data[positionBufferView.byteOffset + positionAccesor.byteOffset];

            // TextureCoord data.
            const tinygltf::Accessor& textureCoordAccesor = model->accessors[primitive.attributes["TEXCOORD_0"]];
            const tinygltf::BufferView& textureCoordBufferView = model->bufferViews[textureCoordAccesor.bufferView];
            const tinygltf::Buffer& textureCoordBuffer = model->buffers[textureCoordBufferView.buffer];
            const int textureCoordBufferStride = textureCoordAccesor.ByteStride(textureCoordBufferView);
            uint8_t const* const texcoords = &textureCoordBuffer.data[textureCoordBufferView.byteOffset + textureCoordAccesor.byteOffset];

            // Normal data.
            const tinygltf::Accessor& normalAccesor = model->accessors[primitive.attributes["NORMAL"]];
            const tinygltf::BufferView& normalBufferView = model->bufferViews[normalAccesor.bufferView];
            const tinygltf::Buffer& normalBuffer = model->buffers[normalBufferView.buffer];
            const int normalByteStride = normalAccesor.ByteStride(normalBufferView);
            uint8_t const* const normals = &normalBuffer.data[normalBufferView.byteOffset + normalAccesor.byteOffset];

            // Tangent data.
            const tinygltf::Accessor& tangentAccesor = model->accessors[primitive.attributes["TANGENT"]];
            const tinygltf::BufferView& tangentBufferView = model->bufferViews[tangentAccesor.bufferView];
            const tinygltf::Buffer& tangentBuffer = model->buffers[tangentBufferView.buffer];
            const int tangentByteStride = tangentAccesor.ByteStride(tangentBufferView);
            uint8_t const* const tangents = &tangentBuffer.data[tangentBufferView.byteOffset + tangentAccesor.byteOffset];

            // Fill in the vertices array.
            for (size_t i : std::views::iota(0u, positionAccesor.count))
            {
                ModelVertex modelVertex{};

                modelVertex.position = {(reinterpret_cast<float const*>(positions + (i * positionByteStride)))[0],
                                        (reinterpret_cast<float const*>(positions + (i * positionByteStride)))[1],
                                        (reinterpret_cast<float const*>(positions + (i * positionByteStride)))[2]};

                modelVertex.textureCoord = {
                    (reinterpret_cast<float const*>(texcoords + (i * textureCoordBufferStride)))[0],
                    (reinterpret_cast<float const*>(texcoords + (i * textureCoordBufferStride)))[1],
                };

                modelVertex.normal = {
                    (reinterpret_cast<float const*>(normals + (i * normalByteStride)))[0],
                    (reinterpret_cast<float const*>(normals + (i * normalByteStride)))[1],
                    (reinterpret_cast<float const*>(normals + (i * normalByteStride)))[2],
                };

                // Required as a model need not have tangents.
                if (tangentAccesor.bufferView)
                {
                    modelVertex.tangent = {
                        (reinterpret_cast<float const*>(tangents + (i * tangentByteStride)))[0],
                        (reinterpret_cast<float const*>(tangents + (i * tangentByteStride)))[1],
                        (reinterpret_cast<float const*>(tangents + (i * tangentByteStride)))[2],
                        (reinterpret_cast<float const*>(tangents + (i * tangentByteStride)))[3],
                    };
                }
                else
                {
                    // note(rtarun9) : This code works, but looks very pixelated in shaders. Utils.hlsli calculates tangents instead.
                    math::XMVECTOR normalVector = math::XMLoadFloat3(&modelVertex.normal);
                    math::XMVECTOR tangentVector = math::XMVector3Cross(normalVector, math::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
                    tangentVector = math::XMVectorLerp(math::XMVector3Cross(normalVector, math::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f)),
                                                       tangentVector,
                                                       math::XMVectorGetX(math::XMVector3Dot(tangentVector, tangentVector)));
                    tangentVector = math::XMVector3Normalize(tangentVector);
                    math::XMFLOAT3 tangent3Vector{};
                    math::XMStoreFloat3(&tangent3Vector, tangentVector);
                    modelVertex.tangent = {tangent3Vector.x, tangent3Vector.y, tangent3Vector.z, 1.0f};
                }

                // Calculate tangent.
                // tangent.z is just a constant (-1 or 1) to indicate the handedness.
                math::XMFLOAT3 tangent3Vector = {modelVertex.tangent.x, modelVertex.tangent.y, modelVertex.tangent.z};
                math::XMVECTOR biTangentVector = math::XMVectorScale(math::XMVector3Cross(XMLoadFloat3(&modelVertex.normal), XMLoadFloat3(&tangent3Vector)), modelVertex.tangent.z);
                math::XMStoreFloat3(&modelVertex.biTangent, biTangentVector);

                vertices.emplace_back(modelVertex);
            }

            // Get the index buffer data.
            tinygltf::BufferView& indexBufferView = model->bufferViews[indexAccesor.bufferView];
            tinygltf::Buffer& indexBuffer = model->buffers[indexBufferView.buffer];
            int indexByteStride = indexAccesor.ByteStride(indexBufferView);
            uint8_t const* const indexes = indexBuffer.data.data() + indexBufferView.byteOffset + indexAccesor.byteOffset;

            // Fill indices array.
            for (size_t i : std::views::iota(0u, indexAccesor.count))
            {
                if (indexAccesor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                {
                    indices.push_back(static_cast<uint32_t>((reinterpret_cast<uint16_t const*>(indexes + (i * indexByteStride)))[0]));
                }
                else if (indexAccesor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                {
                    indices.push_back(static_cast<uint32_t>((reinterpret_cast<uint32_t const*>(indexes + (i * indexByteStride)))[0]));
                }
            }

            const D3D11_BUFFER_DESC vertexBufferDesc = {
                .ByteWidth = static_cast<uint32_t>(vertices.size() * sizeof(ModelVertex)),
                .Usage = D3D11_USAGE_IMMUTABLE,
                .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            };

            const D3D11_SUBRESOURCE_DATA vertexBufferResourceData = {.pSysMem = vertices.data()};

            throwIfFailed(device->CreateBuffer(&vertexBufferDesc, &vertexBufferResourceData, &mesh.vertexBuffer));

            const D3D11_BUFFER_DESC indexBufferDesc = {
                .ByteWidth = static_cast<uint32_t>(indices.size() * sizeof(uint32_t)),
                .Usage = D3D11_USAGE_IMMUTABLE,
                .BindFlags = D3D11_BIND_INDEX_BUFFER,
            };

            const D3D11_SUBRESOURCE_DATA indexBufferResourceData = {.pSysMem = indices.data()};

            throwIfFailed(device->CreateBuffer(&indexBufferDesc, &indexBufferResourceData, &mesh.indexBuffer));

            mesh.indicesCount = static_cast<uint32_t>(indices.size());

            mesh.materialIndex = primitive.material;

            m_meshes.emplace_back(mesh);
        }

        for (const int& childrenNodeIndex : node.children)
        {
            loadNode(device, childrenNodeIndex, model);
        }
    }
}
