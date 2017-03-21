
#ifndef __xml_parser_utils_H__
#define __xml_parser_utils_H__

#include "sf/xml/Node.hpp"

#include <iostream>
#include <string>

namespace sf {
namespace xml {

Node* parse (std::istream &reader, bool treatAttributesAsChildren);
Node* parse (std::string filename, bool treatAttributesAsChildren);
Node* parseString (std::string xmlString, bool treatAttributesAsChildren);
bool isWhitespace(char ch);

std::string readChunk(std::istream &reader);
void subChars(std::string &srcStr);
std::string putAttributes(std::string str, Node* node, bool treatAsChildren);
bool startsWith(std::string str, std::string search);
bool endsWith(std::string str, std::string search);

}
}

#endif
