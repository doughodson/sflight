
#ifndef __init_InverseDesign_HPP__
#define __init_InverseDesign_HPP__

namespace sflight {

namespace xml  { class Node; }
namespace mdls { class InverseDesign; }
namespace xml_bindings {
void init_InverseDesign(sflight::xml::Node*, sflight::mdls::InverseDesign*);
}

}

#endif
