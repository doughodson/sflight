
#ifndef __init_EOMFiveDOF_H__
#define __init_EOMFiveDOF_H__

namespace sflight {
namespace xml {
class Node;
}
namespace mdls {
class EOMFiveDOF;
}
namespace xml_bindings {

void init_EOMFiveDOF(sflight::xml::Node*, sflight::mdls::EOMFiveDOF*);
}
}

#endif
