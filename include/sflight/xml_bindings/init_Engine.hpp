
#ifndef __init_Engine_H__
#define __init_Engine_H__

namespace sflight {
namespace xml {
class Node;
}
namespace mdls {
class Engine;
}
namespace xml_bindings {

void init_Engine(sflight::xml::Node*, sflight::mdls::Engine*);
}
}

#endif
