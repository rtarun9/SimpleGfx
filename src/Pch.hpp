#pragma once

#include <string_view>
#include <span>
#include <vector>
#include <iostream>
#include <exception>
#include <array>
#include <format>
#include <source_location>

#include <d3d11.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <d3dcompiler.h>

#include "Utils.hpp"
#include "Types.hpp"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dxguid.lib")