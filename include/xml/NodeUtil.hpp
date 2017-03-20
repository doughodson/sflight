#ifndef NODEUTIL_HPP_
#define NODEUTIL_HPP_

#include <string>
#include <vector>
#include "Node.hpp"

using std :: string;

namespace sf {

	class NodeUtil {
		
		public:
		
		/** traverses the parent's childList and returns a new Node
		 *  that contains only the children that have the specified pathName.
		 *  All children are added to the root of the childList
		 */
		static vector<Node*> getList(Node* parent, string childName);
		
		static string get(Node* node, string pathName, string defaultVal);
		static int getInt(Node* node, string pathName, int defaultVal);
		static long getLong(Node* node, string pathName, long defaultVal);
		static float getFloat(Node* node, string pathName, float defaultVal);
		static double getDouble(Node* node, string pathName, double defaultVal);
		static bool getBool(Node* node, string pathName, bool defaultVal);
		static vector<string> splitString(string instr, char splitChar);

		
		
	};	
	
}

#endif /*NODEUTIL_HPP_*/
