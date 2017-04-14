
#ifndef __sflight_mdls_load_modules_H__
#define __sflight_mdls_load_modules_H__

namespace sflight {
namespace xml {
class Node;
}
namespace mdls {
class Player;

void load_modules(xml::Node* node, Player*);
}
}

#endif
