
#ifndef __init_InterpAero_HPP__
#define __init_InterpAero_HPP__

namespace sflight {

namespace xml  { class Node; }
namespace mdls { class InterpAero; }
namespace xml_bindings {
void init_InterpAero(sflight::xml::Node*, sflight::mdls::InterpAero*);
}

}

#endif
