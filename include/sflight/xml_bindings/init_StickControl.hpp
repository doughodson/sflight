
#ifndef __init_StickControl_HPP__
#define __init_StickControl_HPP__

namespace sflight {

namespace xml  { class Node; }
namespace mdls { class StickControl; }
namespace xml_bindings {
void init_StickControl(sflight::xml::Node*, sflight::mdls::StickControl*);
}

}

#endif
