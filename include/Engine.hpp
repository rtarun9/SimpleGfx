#pragma once

#include "Application.hpp"

class Engine final : public sgfx::Application
{
  public:
    Engine(const std::string_view windowTitle);

    void loadContent() override;
    void update() override;
    void render() override;

  private:
    sgfx::GraphicsPipeline m_pipeline{};

    comptr<ID3D11Buffer> m_vertexBuffer{};
};
