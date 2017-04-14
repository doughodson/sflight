
#ifndef __sflight_xml_bindings_builder_H__
#define __sflight_xml_bindings_builder_H__

namespace sflight {
namespace xml {
class Node;
}
namespace mdls {
class Player;
}
namespace xml_bindings {

void builder(xml::Node*, mdls::Player*);
}
}

#endif
