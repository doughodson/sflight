   -- models
   project "mdls"
      kind "StaticLib"
      files {
         "../../include/sflight/mdls/**.h*",
         "../../src/mdls/**.cpp"
      }
      targetname "sflight_mdls"

   -- xml parser
   project "xml"
      kind "StaticLib"
      files {
         "../../include/sflight/xml/**.h*",
         "../../src/xml/**.cpp"
      }
      targetname "sflight_xml"

   -- xml bindings
   project "xml_bindings"
      kind "StaticLib"
      files {
         "../../include/sflight/xml_bindings/**.h*",
         "../../src/xml_bindings/**.cpp"
      }
      targetname "sflight_xml_bindings"

