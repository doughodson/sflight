
#ifndef __init_EOMFiveDOF_HPP__
#define __init_EOMFiveDOF_HPP__

namespace sflight {

namespace xml  { class Node; }
namespace mdls { class EOMFiveDOF; }
namespace xml_bindings {
void init_EOMFiveDOF(sflight::xml::Node*, sflight::mdls::EOMFiveDOF*);
}

}

#endif
