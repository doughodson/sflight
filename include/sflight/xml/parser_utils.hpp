
#ifndef __sflight_xml_parser_utils_HPP__
#define __sflight_xml_parser_utils_HPP__

#include "sflight/xml/Node.hpp"

#include <iostream>
#include <string>

namespace sflight {
namespace xml {

// parse a given filename
Node* parse(const std::string& filename, const bool treatAttributesAsChildren);

// parse input stream
Node* parse(std::istream&, const bool treatAttributesAsChildren);

std::string readChunk(std::istream&);

// utility functions
bool isWhitespace(const char);
void subChars(std::string&);
std::string putAttributes(std::string, Node*, const bool treatAsChildren);
bool startsWith(const std::string& str, const std::string& search);
bool endsWith(const std::string& str, const std::string& search);

}
}

#endif
