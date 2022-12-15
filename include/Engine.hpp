#pragma once

#include "Application.hpp"

class Engine final : public sgfx::Application
{
  public:
    Engine(const std::string_view windowTitle);

    void loadContent() override;
    void update(const float deltaTime) override;
    void render() override;

  private:
    comptr<ID3D11DepthStencilView> m_dsv{};
    sgfx::RenderTarget m_offscreenRT{};
    comptr<ID3D11SamplerState> m_offscreenSampler{};

    sgfx::GraphicsPipeline m_pipeline{};
    sgfx::GraphicsPipeline m_lightPipeline{};
    sgfx::GraphicsPipeline m_fullscreenPassPipeline{};

    sgfx::ConstantBuffer<sgfx::SceneBuffer> m_sceneBuffer{};

    std::unordered_map<std::string, sgfx::Model> m_renderables{};

    sgfx::Model m_lightModel{};
    sgfx::ConstantBuffer<sgfx::LightMatrix> m_lightMatricesBuffer{};
    std::array<math::XMFLOAT4, sgfx::LIGHT_COUNT - 1u> m_lightPositions{};

    std::array<sgfx::RenderTarget, 3> m_gpassRts{};
    sgfx::GraphicsPipeline m_gpassPipeline{};
    comptr<ID3D11DepthStencilView> m_gpassDsv{};

    float m_sunAngle{123.0f};
};
