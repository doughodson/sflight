
#ifndef __init_AutoPilot_HPP__
#define __init_AutoPilot_HPP__

namespace sflight {

namespace xml  { class Node; }
namespace mdls { class AutoPilot; }
namespace xml_bindings {
void init_AutoPilot(sflight::xml::Node*, sflight::mdls::AutoPilot*);
}

}

#endif
