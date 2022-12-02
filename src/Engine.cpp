#include "Engine.hpp"

#include <array>

Engine::Engine(const std::string_view windowTitle)
	: sgfx::Application(windowTitle)
{
}

void Engine::loadContent()
{
}

void Engine::update()
{
}

void Engine::render()
{
	constexpr std::array<float, 4> clearColor{ 0.2f, 0.2f, 0.2f, 1.0f };

	m_deviceContext->ClearRenderTargetView(m_renderTargetView.Get(), clearColor.data());
	m_deviceContext->RSSetViewports(1u, &m_viewport);

	m_deviceContext->OMSetRenderTargets(1u, m_renderTargetView.GetAddressOf(), nullptr);

	throwIfFailed(m_swapchain->Present(1u, 0u));
}


