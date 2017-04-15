
#ifndef __init_TableAero_H__
#define __init_TableAero_H__

namespace sflight {
namespace xml {
class Node;
}
namespace mdls {
class TableAero;
}
namespace xml_bindings {

void init_TableAero(sflight::xml::Node*, sflight::mdls::TableAero*);
}
}

#endif
