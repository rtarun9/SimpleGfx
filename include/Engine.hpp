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
    sgfx::DepthTexture m_depthTexture{};
    sgfx::RenderTarget m_offscreenRT{};
    comptr<ID3D11SamplerState> m_offscreenSampler{};
    comptr<ID3D11SamplerState> m_wrapSampler{};

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
    sgfx::DepthTexture m_gpassDepthTexture{};

    comptr<ID3D11ShaderResourceView> m_ssaoRandomRotationTexture{};
    sgfx::GraphicsPipeline m_ssaoPipeline{};
    sgfx::GraphicsPipeline m_boxBlurPipeline{};
    sgfx::RenderTarget m_ssaoRt{};
    sgfx::RenderTarget m_ssaoBlurredRt{};
    sgfx::ConstantBuffer<sgfx::SSAOBuffer> m_ssaoBuffer{};

    float m_sunAngle{123.0f};
};
