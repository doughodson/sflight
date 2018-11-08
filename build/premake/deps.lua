
--
-- dependency libraries
--

-- lua library
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
   excludes {
      "../../deps/lua/src/lua.c",
      "../../deps/lua/src/luac.c"
   }
   if os.ishost("linux") then
      defines { "LUA_COMPAT_MODULE_5_2", "LUA_USE_LINUX" }
   end
   targetname "lua"

