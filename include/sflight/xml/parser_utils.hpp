
#ifndef __sflight_xml_parser_utils_HPP__
#define __sflight_xml_parser_utils_HPP__

#include "sflight/xml/Node.hpp"

#include <iostream>
#include <string>

namespace sflight {
namespace xml {

// parse a given xml structured file and return a Node 'tree'
Node* parse(const std::string& filename, const bool treatAttributesAsChildren);

// parse a given input stream and return a Node 'tree'
Node* parse(std::istream&, const bool treatAttributesAsChildren);

// read in a sequence of characters from an input stream up to '>' and return a string 'chunk'
std::string readChunk(std::istream&);

// determine if a character is a whitespace
bool isWhitespace(const char);

std::string putAttributes(const std::string&, Node* const, const bool treatAsChildren);

// search a string to see if it starts with another string
bool startsWith(const std::string& str, const std::string& search);
// search a string to see if it end with another string
bool endsWith(const std::string& str, const std::string& search);

}
}

#endif
