workspace "SimpleGfx"
    configurations 
    { 
        "Debug", 
        "Release" 
    }
    architecture "x86_64"

project "SimpleGfx"
    kind "ConsoleApp"
    
    language "C++"
    cppdialect "C++20"
    
    targetdir "bin/%{cfg.buildcfg}"
    
    staticruntime "Off"

    nuget 
    { 
        "sdl2.v140:2.0.4", 
        "sdl2.v140.redist:2.0.4",
        "directxtex_desktop_2019:2022.10.18.1"
    }

    pchheader "Pch.hpp"
    pchsource "src/Pch.cpp"

    files 
    { 
        "src/**.cpp",
        "include/**.hpp",
        "shaders/**.hlsl",
        "shaders/**.hlsli",
    }

    includedirs
    {
        "include/"
    }

    links
    {
        "d3d11.lib",
        "dxgi.lib",
        "d3dcompiler.lib",
        "winmm.lib",
        "dxguid.lib"
    }

    filter "files:**.hlsl"
        buildaction ("None")

    filter "configurations:Debug"
        defines 
        { 
            "_DEBUG" 
        }
        symbols "On"
        optimize "Debug"

    filter "configurations:Release"
        defines 
        { 
            "_NDEBUG"
        }
        optimize "Speed"