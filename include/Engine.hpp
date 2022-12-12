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

    sgfx::GraphicsPipeline m_pipeline{};
    sgfx::GraphicsPipeline m_lightPipeline{};

    sgfx::ConstantBuffer<sgfx::SceneBuffer> m_sceneBuffer{};

    std::unordered_map<std::string, sgfx::Model> m_renderables{};
    sgfx::Model m_light{};
};
