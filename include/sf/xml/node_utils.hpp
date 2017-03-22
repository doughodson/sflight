
#ifndef __sf_xml_node_utils_H__
#define __sf_xml_node_utils_H__

#include "sf/xml/Node.hpp"

#include <string>
#include <vector>

namespace sf {
namespace xml {

//
// traverses the parent's childList and returns a new Node
// that contains only the children that have the specified pathName.
// All children are added to the root of the childList
//
std::vector<Node*> getList(Node* const parent, const std::string childName);

std::string get(Node* const, const std::string pathName,
                const std::string defaultVal);

int getInt(Node* const, const std::string pathName, const int defaultVal);

long getLong(Node* const, const std::string pathName, const long defaultVal);

float getFloat(Node* const, const std::string pathName, const float defaultVal);

double getDouble(Node* const, const std::string pathName,
                 const double defaultVal);

bool getBool(Node* const, const std::string pathName, const bool defaultVal);

std::vector<std::string> splitString(const std::string instr,
                                     const char splitChar);
}
}

#endif
