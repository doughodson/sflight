
#ifndef __ModuleLoader_H__
#define __ModuleLoader_H__

namespace sf
{
class Node;
class FDMGlobals;

//------------------------------------------------------------------------------
// Class: ModuleLoader
//------------------------------------------------------------------------------
class ModuleLoader
{
 public:
   static void loadModules(Node* node, FDMGlobals* globals);
};

}

#endif
