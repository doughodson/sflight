--
-- If premake command is not supplied an action (target compiler), exit!
--
-- Target of interest:
--     vs2017     (Visual Studio 2017)
--     gmake      (Linux make)
--

-- we must have an ide/compiler specified
if (_ACTION == nil) then
  return
end

workspace "sflight"

   -- destination directory for generated solution/project files
   location ("../" .. _ACTION)

   -- C++ code in all projects
   language "C++"

   -- common include directories (all configurations/all projects)
   includedirs { "../../include" }

   --
   -- Build (solution) configuration options:
   --     Release        (Application linked to Multi-threaded DLL)
   --     Debug          (Application linked to Multi-threaded Debug DLL)
   --
   configurations { "Release", "Debug" }

   -- visual studio options and warnings
   -- /wd4351 (C4351 warning) - disable warning associated with array brace initialization
   -- /Oi - generate intrinsic functions
   -- buildoptions( { "/wd4351", "/Oi" } )

   -- common release configuration flags and symbols
   filter { "Release" }
      optimize "On"
      if _ACTION ~= "gmake" then
         -- favor speed over size
         buildoptions { "/Ot" }
         defines { "WIN32", "NDEBUG" }
      end

   -- common debug configuration flags and symbols
   filter { "Debug" }
      symbols "On"
      targetsuffix "_d"
      if _ACTION ~= "gmake" then
         defines { "WIN32", "_DEBUG" }
      end

   -- models
   project "mdls"
      kind "StaticLib"
      files {
         "../../include/sflight/mdls/**.h*",
         "../../src/mdls/**.cpp"
      }
      targetdir "../../lib/"
      targetname "sflight_mdls"

   -- xml parser
   project "xml"
      kind "StaticLib"
      files {
         "../../include/sflight/xml/**.h*",
         "../../src/xml/**.cpp"
      }
      targetdir "../../lib/"
      targetname "sflight_xml"

   -- xml bindings
   project "xml_bindings"
      kind "StaticLib"
      files {
         "../../include/sflight/xml_bindings/**.h*",
         "../../src/xml_bindings/**.cpp"
      }
      targetdir "../../lib/"
      targetname "sflight_xml_bindings"

   -- simple application
   project "mainTest"
      kind "ConsoleApp"
      targetname "mainTest"
      targetdir "../../examples/mainTest"
      debugdir "../../examples/mainTest"
      files {
         "../../examples/mainTest/**.h*",
         "../../examples/mainTest/**.cpp"
      }
      links { "xml_bindings", "xml", "mdls" }
      libdirs { "../../lib" }
--      if _ACTION ~= "gmake" then
--         defines { "_CONSOLE" }
--      end
      filter "configurations:Release"
         if _ACTION ~= "gmake" then
            links { "Ws2_32", "Winmm", "comctl32", "gdi32" }
         end
      filter "configurations:Debug"
         if _ACTION ~= "gmake" then
            links { "Ws2_32", "Winmm", "comctl32", "gdi32" }
         end

