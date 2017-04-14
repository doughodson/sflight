
#ifndef __sflight_xml_bindings_load_modules_H__
#define __sflight_xml_bindings_load_modules_H__

namespace sflight {
namespace xml {
class Node;
}
namespace mdls {
class Player;
}
namespace xml_bindings {

void load_modules(xml::Node*, mdls::Player*);
}
}

#endif
