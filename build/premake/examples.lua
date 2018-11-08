
-- test of simpleflight
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
   if _ACTION == "gmake" then
      buildoptions "-std=c++14"
   else
      links { "Ws2_32", "Winmm", "comctl32", "gdi32" }
   end

-- lua interpreter
project "lua-repl"
   kind "ConsoleApp"
   targetname "lua-repl"
   targetdir "../../examples/lua"
   debugdir "../../examples/lua"
   includedirs { "../../deps/lua/include" }
   files {
      "../../examples/lua/**.h*",
      "../../examples/lua/**.c"
   }
   links { "lua" }
   libdirs { "../../lib" }
   if _ACTION == "gmake" then
      links { "dl", "readline" }
   else
      links { "Ws2_32", "Winmm", "comctl32", "gdi32" }
   end

-- luac compiler
project "luac"
   kind "ConsoleApp"
   targetname "luac"
   targetdir "../../examples/luac"
   debugdir "../../examples/luac"
   includedirs { "../../deps/lua/include" }
   files {
      "../../examples/luac/**.h*",
      "../../examples/luac/**.c"
   }
   links { "lua" }
   libdirs { "../../lib" }
   if _ACTION == "gmake" then
      links { "dl", "readline" }
   else
      links { "Ws2_32", "Winmm", "comctl32", "gdi32" }
   end

-- clips interpreter
project "clips-repl"
   kind "ConsoleApp"
   targetname "clips-repl"
   targetdir "../../examples/clips"
   debugdir "../../examples/clips"
   includedirs { "../../deps/clips/include" }
   files {
      "../../examples/clips/**.h*",
      "../../examples/clips/**.c"
   }
   links { "clips" }
   libdirs { "../../lib" }
   if _ACTION ~= "gmake" then
      links { "Ws2_32", "Winmm", "comctl32", "gdi32" }
   end
