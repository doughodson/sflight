
#ifndef __XMLParser_H__
#define __XMLParser_H__

#include "Node.hpp"

#include <iostream>

namespace sf {

//------------------------------------------------------------------------------
// Class: XMLParser
//------------------------------------------------------------------------------
class XMLParser
{
public:
   XMLParser();
   ~XMLParser();

   static Node* parse (std::istream &reader, bool treatAttributesAsChildren);
   static Node* parse (std::string filename, bool treatAttributesAsChildren);
   static Node* parseString (std::string xmlString, bool treatAttributesAsChildren);
   static bool isWhitespace(char ch);

private:
   static std::string readChunk(std::istream &reader);
   static void subChars(std::string &srcStr);
   static std::string putAttributes(std::string str, Node* node, bool treatAsChildren);
   static bool startsWith(std::string str, std::string search);
   static bool endsWith(std::string str, std::string search);
};

}

#endif
