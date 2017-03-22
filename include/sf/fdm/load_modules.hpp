
#ifndef __loadModules_H__
#define __loadModules_H__

namespace sf {
namespace xml {
class Node;
}
namespace fdm {
class FDMGlobals;

void load_modules(xml::Node* node, FDMGlobals* globals);
}
}

#endif
