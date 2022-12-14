#pragma once

#ifdef _DEBUG
constexpr bool SGFX_DEBUG = true;
#else
constexpr bool SGFX_DEBUG = false;
#endif

#include <array>
#include <exception>
#include <format>
#include <iostream>
#include <source_location>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <ranges>

#include <d3d11.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <d3dcompiler.h>

#include <DirectXMath.h>

// Global namespace aliases.
namespace wrl = Microsoft::WRL;
namespace math = DirectX;

#include "Types.hpp"
#include "Utils.hpp"
