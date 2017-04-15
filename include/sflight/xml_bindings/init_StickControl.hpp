
#ifndef __init_StickControl_H__
#define __init_StickControl_H__

namespace sflight {
namespace xml {
class Node;
}
namespace mdls {
class StickControl;
}
namespace xml_bindings {

void init_StickControl(sflight::xml::Node*, sflight::mdls::StickControl*);
}
}

#endif
