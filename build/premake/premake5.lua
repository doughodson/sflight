--
-- If premake command is not supplied an action (target compiler), exit!
--
-- Target of interest:
--     vs2017     (Visual Studio 2017)
--     vs2019     (Visual Studio 2019)
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

   -- destination directory for compiled binary target
   targetdir("../../lib/")

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


   -- libs
   dofile "deps.lua"
   dofile "sflight.lua"

  -- examples
   dofile "examples.lua"
