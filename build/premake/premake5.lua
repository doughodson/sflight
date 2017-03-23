--
-- If premake command is not supplied an action (target compiler), exit!
--
-- Target of interest:
--     vs2013     (Visual Studio 2013)
--     vs2015     (Visual Studio 2015)
--     vs2017     (Visual Studio 2017)
--

-- we must have an ide/compiler specified
if (_ACTION == nil) then
  return
end

workspace "sflt"

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
   configurations { "Release32", "Debug32" }

   -- common release configuration flags and symbols
   filter { "Release32" }
      flags { "Optimize" }
      -- favor speed over size
      buildoptions { "/Ot" }
      defines { "WIN32", "NDEBUG" }

   -- common debug configuration flags and symbols
   filter { "Debug32" }
      symbols "On"
      targetsuffix "_d"
      defines { "WIN32", "_DEBUG" }

   -- xml parser
   project "xml"
      kind "StaticLib"
      files {
         "../../include/sf/xml/**.h*",
         "../../src/xml/**.cpp"
      }
      targetdir ("../../lib/")
      targetname "sflt_xml"

   -- flight dynamics model
   project "fdm"
      kind "StaticLib"
      files {
         "../../include/sf/fdm/**.h*",
         "../../src/fdm/**.cpp"
      }
      targetdir ("../../lib/")
      targetname "sflt_fdm"

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
      links { "xml", "fdm" }
      libdirs { "../../lib" }
      defines { "_CONSOLE" }
      filter "configurations:Release*"
         links { "Ws2_32", "Winmm", "comctl32", "gdi32"}
      filter "configurations:Debug*"
         links { "Ws2_32", "Winmm", "comctl32", "gdi32" }

