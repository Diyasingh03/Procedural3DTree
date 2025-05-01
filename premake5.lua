-- run premake command to generate a solution file in the temp folder
--  premake5 vs2017
--  premake5 gmake2
--  premake5 xcode4

function os.winSdkVersion()
    -- fix for vs2017 incorrectly selecting 8.1 SDK when the SDK is not installed.
    -- This is patched in latest premake:master branch but not released on the premake website.
    -- In the patched version of premake, use systemversion("latest")
    -- https://github.com/premake/premake-core/issues/935
    
    local reg_arch = iif( os.is64bit(), "\\Wow6432Node\\", "\\" )
    local sdk_version = os.getWindowsRegistry( "HKLM:SOFTWARE" .. reg_arch .."Microsoft\\Microsoft SDKs\\Windows\\v10.0\\ProductVersion" )
    if sdk_version ~= nil then return sdk_version end
end

-- CONFIGURATION VARIABLES (this is where we want the generated solution to put its files, or look for source code)
binaries_folder = "binaries/"
includes_folder = "include/"
source_folder   = "source/"                         
libs_folder     = "libs/"

workspace "Plant Generator"
    location("temp/") -- temporary files (sln, proj, obj, pdb, ilk, etc)
    language "C++"

    configurations { "Debug", "Release" }

    cppdialect "C++17"
    systemversion(os.winSdkVersion() .. ".0")
    system      "windows"
    platforms { "win64" }
    defines   { "OS_WINDOWS" }        

    filter { "platforms:*64"} architecture "x64"

    entrypoint "mainCRTStartup"     -- force Windows-executables to use main instead of WinMain as entry point   
    symbolspath '$(TargetName).pdb'
    staticruntime "on"

    debugdir(binaries_folder)
    includedirs { includes_folder }
    libdirs     { libs_folder }
    links       { "opengl32", "SDL2" }
    flags       { "MultiProcessorCompile" }

    filter { "configurations:Debug" }
        defines { "DEBUG" }
        symbols "On"
        optimize "Off"
        
    filter { "configurations:Release" }
        defines { "NDEBUG" }
        symbols "Off"        
        optimize "On"
        
    filter{}


project "L-system 3D"
    kind "ConsoleApp"
    targetdir(binaries_folder)
    targetname("tree3d")
    files ({source_folder .. "**.h", source_folder .. "**.c", source_folder .. "**.cpp"})
    removefiles{ source_folder .. "main*.cpp"}
    files ({source_folder .. "main_3d.cpp"})
    
project "L-system 2D"
    kind "ConsoleApp"
    targetdir(binaries_folder)
    targetname("fractals2d")
    files ({source_folder .. "**.h", source_folder .. "**.c", source_folder .. "**.cpp"})
    removefiles{ source_folder .. "main*.cpp"}
    files ({source_folder .. "main_2d.cpp"})
    