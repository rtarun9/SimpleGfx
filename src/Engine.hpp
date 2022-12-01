#pragma once

#include "Application.hpp"

class Engine final : public sgfx::Application
{
public:
	Engine(const std::string_view windowTitle);

	void loadContent() override;
	void render() override;
	void update() override;

private:
};

