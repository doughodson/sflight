
#ifndef __init_InterpAero_H__
#define __init_InterpAero_H__

namespace sflight {
namespace xml {
class Node;
}
namespace mdls {
class InterpAero;
}
namespace xml_bindings {

void init_InterpAero(sflight::xml::Node*, sflight::mdls::InterpAero*);
}
}

#endif
