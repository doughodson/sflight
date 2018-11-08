
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

