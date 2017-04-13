
#ifndef __sflight_fdm_load_modules_H__
#define __sflight_fdm_load_modules_H__

namespace sflight {
namespace xml {
class Node;
}
namespace fdm {
class Player;

void load_modules(xml::Node* node, Player*);
}
}

#endif
