
#ifndef __init_InverseDesign_H__
#define __init_InverseDesign_H__

namespace sflight {
namespace xml {
class Node;
}
namespace mdls {
class InverseDesign;
}
namespace xml_bindings {

void init_InverseDesign(sflight::xml::Node*, sflight::mdls::InverseDesign*);
}
}

#endif
