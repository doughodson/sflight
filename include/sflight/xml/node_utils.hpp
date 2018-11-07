
#ifndef __sflight_xml_node_utils_HPP__
#define __sflight_xml_node_utils_HPP__

#include "sflight/xml/Node.hpp"

#include <string>
#include <vector>

namespace sflight {
namespace xml {

//
// traverses the parent's childList and returns a new Node
// that contains only the children that have the specified pathName.
// All children are added to the root of the childList
//
std::vector<Node*> getList(Node* const parent, const std::string& childName);

std::string getString(Node* const, const std::string& pathName, const std::string& defaultVal);

int getInt(Node* const, const std::string& pathName, const int defaultVal);

long getLong(Node* const, const std::string& pathName, const long defaultVal);

float getFloat(Node* const, const std::string& pathName, const float defaultVal);

double getDouble(Node* const, const std::string& pathName, const double defaultVal);

bool getBool(Node* const, const std::string& pathName, const bool defaultVal);

std::vector<std::string> splitString(const std::string& instr, const char splitChar);
}
}

#endif
