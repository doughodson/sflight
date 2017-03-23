
#ifndef __sflt_xml_parser_utils_H__
#define __sflt_xml_parser_utils_H__

#include "sflt/xml/Node.hpp"

#include <iostream>
#include <string>

namespace sflt {
namespace xml {

Node* parse (std::istream& reader, const bool treatAttributesAsChildren);
Node* parse (const std::string filename, const bool treatAttributesAsChildren);
Node* parseString (const std::string& xmlString, const bool treatAttributesAsChildren);
bool isWhitespace(const char ch);

std::string readChunk(std::istream& reader);
void subChars(std::string& srcStr);
std::string putAttributes(std::string str, Node* node, const bool treatAsChildren);
bool startsWith(const std::string str, const std::string search);
bool endsWith(const std::string str, const std::string search);

}
}

#endif
