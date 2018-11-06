
#ifndef __init_Player_HPP__
#define __init_Player_HPP__

namespace sflight {

namespace xml  { class Node; }
namespace mdls { class Player; }
namespace xml_bindings {
void init_Player(sflight::xml::Node*, sflight::mdls::Player*);
}

}

#endif
