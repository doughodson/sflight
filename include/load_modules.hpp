
#ifndef __loadModules_H__
#define __loadModules_H__

namespace xml { class Node; }
namespace sf
{
class FDMGlobals;

void load_modules(xml::Node* node, FDMGlobals* globals);

}

#endif
