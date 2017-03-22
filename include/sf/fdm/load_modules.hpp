
#ifndef __sf_fdm_load_modules_H__
#define __sf_fdm_load_modules_H__

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
