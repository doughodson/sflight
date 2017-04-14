
#ifndef __init_AutoPilot_H__
#define __init_AutoPilot_H__

namespace sflight {
namespace xml {
class Node;
}
namespace mdls {
class AutoPilot;
}
namespace xml_bindings {

void init_AutoPilot(sflight::xml::Node*, sflight::mdls::AutoPilot*);
}
}

#endif
