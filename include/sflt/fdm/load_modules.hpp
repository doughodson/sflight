
#ifndef __sflt_fdm_load_modules_H__
#define __sflt_fdm_load_modules_H__

namespace sflt {
namespace xml {
class Node;
}
namespace fdm {
class FDMGlobals;

void load_modules(xml::Node* node, FDMGlobals* globals);
}
}

#endif
