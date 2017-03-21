
#ifndef __node_utils_H__
#define __node_utils_H__

#include "xml/Node.hpp"

#include <string>
#include <vector>

namespace xml {

//
// traverses the parent's childList and returns a new Node
// that contains only the children that have the specified pathName.
// All children are added to the root of the childList
//
std::vector<Node*> getList(Node* parent, std::string childName);

std::string get(Node* node, std::string pathName, std::string defaultVal);
int getInt(Node* node, std::string pathName, int defaultVal);
long getLong(Node* node, std::string pathName, long defaultVal);
float getFloat(Node* node, std::string pathName, float defaultVal);
double getDouble(Node* node, std::string pathName, double defaultVal);
bool getBool(Node* node, std::string pathName, bool defaultVal);
std::vector<std::string> splitString(std::string instr, char splitChar);


}

#endif
