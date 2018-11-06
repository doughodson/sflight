
#ifndef __init_WaypointFollower_HPP__
#define __init_WaypointFollower_HPP__

namespace sflight {

namespace xml  { class Node; }
namespace mdls { class WaypointFollower; }
namespace xml_bindings {
void init_WaypointFollower(sflight::xml::Node*, sflight::mdls::WaypointFollower*);
}

}

#endif
