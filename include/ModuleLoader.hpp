
#ifndef MODULE_LOADER_H
#define MODULE_LOADER_H

#include <string>

using std :: string;

namespace sf {

	class Node;
	class FDMGlobals;

	class ModuleLoader {

	public:

		static void loadModules(Node* node, FDMGlobals *globals);


	};



}


#endif
