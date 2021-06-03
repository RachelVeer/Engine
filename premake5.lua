workspace "Engine"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release"
	}

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["ImGui"] = "Engine/vendor/imgui"

--group "Dependencies"
	--include "Engine/vendor/imgui"

group ""

project "Engine"
	location "Engine"
	kind     "StaticLib"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir    ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/src/**.hlsl"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.ImGui}"
	}

	links
	{
		"ImGui"
	}


	filter 'files:**.hlsl'
   -- A message to display while this build step is running (optional)
   buildmessage 'Compiling %{file.relpath}'

   -- One or more commands to run (required)
   buildcommands {
      'copy %(Identity) "$(OutDir)/../Sandbox/src" > NUL'
   }

   -- One or more outputs resulting from the build (required)
   buildoutputs { '$(OutDir)/../Sandbox/%(Identity)' }


filter "system:windows"
	cppdialect "C++17"
	staticruntime "On"
	systemversion "latest"

	defines
	{
		"ENGINE_PLATFORM_WINDOWS"
	}

filter "configurations:Debug"
	defines "DEBUG"
	symbols "On"

filter "configurations:Release"
	defines "NDEBUG"
	optimize "On"

project "Sandbox"
	location "Sandbox"
	kind     "ConsoleApp"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir    ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"Engine/vendor/spdlog/include",
		"Engine/vendor/imgui",
		"Engine/vendor/",
		"Engine/src"
	}

	links
	{
		"Engine"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"ENGINE_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "NDEBUG"
		optimize "On"

project "ImGui"
	location "Engine/vendor/imgui"
	kind "StaticLib"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{IncludeDir.ImGui}/imconfig.h",
		"%{IncludeDir.ImGui}/imgui.h",
		"%{IncludeDir.ImGui}/imgui.cpp",
		"%{IncludeDir.ImGui}/imgui_draw.cpp",
		"%{IncludeDir.ImGui}/imgui_internal.h",
		"%{IncludeDir.ImGui}/imgui_widgets.cpp",
		"%{IncludeDir.ImGui}/imstb_rectpack.h",
		"%{IncludeDir.ImGui}/imstb_textedit.h",
		"%{IncludeDir.ImGui}/imstb_truetype.h",
		"%{IncludeDir.ImGui}/imgui_demo.cpp",
		"%{IncludeDir.ImGui}/imgui_tables.cpp",
		"%{IncludeDir.ImGui}/backends/imgui_impl_win32.h",
		"%{IncludeDir.ImGui}/backends/imgui_impl_win32.cpp",
		"%{IncludeDir.ImGui}/backends/imgui_impl_dx12.h",
		"%{IncludeDir.ImGui}/backends/imgui_impl_dx12.cpp"
	}

	includedirs 
	{
		"%{IncludeDir.ImGui}"
	}

	filter "system:windows"
		systemversion "latest"
		cppdialect "C++17"
		staticruntime "On"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		runtime "Release"
		optimize "On"