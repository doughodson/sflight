
--
-- dependency libraries
--

-- lua
project "lua"
   language "C"
   kind "StaticLib"
   -- include directories
   includedirs { "../../deps/lua/include" }
   files {
      "../../deps/lua/include/**.h",
      "../../deps/lua/include/**.hpp",
      "../../deps/lua/src/**.c"
   }
   if os.ishost("linux") then
      defines { "LUA_COMPAT_MODULE_5_2", "LUA_USE_LINUX" }
   end
   targetname "lua"

-- clips
project "clips"
   language "C"
   kind "StaticLib"
   -- include directories
   includedirs { "../../deps/clips/include" }
   files {
      "../../deps/clips/include/**.h",
      "../../deps/clips/include/**.hpp",
      "../../deps/clips/src/**.c"
   }
   targetname "clips"

