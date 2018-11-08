-- models
project "mdls"
   kind "StaticLib"
   if _ACTION == "gmake" then
      buildoptions "-std=c++14"
   end
   files {
      "../../include/sflight/mdls/**.h*",
      "../../src/mdls/**.cpp"
   }
   targetname "sflight_mdls"

-- xml parser
project "xml"
   kind "StaticLib"
   if _ACTION == "gmake" then
      buildoptions "-std=c++14"
   end
   files {
      "../../include/sflight/xml/**.h*",
      "../../src/xml/**.cpp"
   }
   targetname "sflight_xml"

-- xml bindings
project "xml_bindings"
   kind "StaticLib"
   if _ACTION == "gmake" then
      buildoptions "-std=c++14"
   end
   files {
      "../../include/sflight/xml_bindings/**.h*",
      "../../src/xml_bindings/**.cpp"
   }
   targetname "sflight_xml_bindings"

