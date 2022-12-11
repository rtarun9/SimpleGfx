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
    sgfx::GraphicsPipeline m_pipeline{};

    sgfx::ConstantBuffer<sgfx::SceneBuffer> m_sceneBuffer{};

    sgfx::Model m_cube{};

    comptr<ID3D11DepthStencilView> m_dsv{};
};
