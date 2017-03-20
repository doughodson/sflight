
#ifndef __NodeUtil_H__
#define __NodeUtil_H__

#include "Node.hpp"

#include <string>
#include <vector>

namespace sf {

class NodeUtil
{
public:
   //
   // traverses the parent's childList and returns a new Node
	// that contains only the children that have the specified pathName.
	// All children are added to the root of the childList
	//
   static std::vector<Node*> getList(Node* parent, std::string childName);
		
   static std::string get(Node* node, std::string pathName, std::string defaultVal);
   static int getInt(Node* node, std::string pathName, int defaultVal);
   static long getLong(Node* node, std::string pathName, long defaultVal);
   static float getFloat(Node* node, std::string pathName, float defaultVal);
   static double getDouble(Node* node, std::string pathName, double defaultVal);
   static bool getBool(Node* node, std::string pathName, bool defaultVal);
   static std::vector<std::string> splitString(std::string instr, char splitChar);

};	
	
}

#endif
