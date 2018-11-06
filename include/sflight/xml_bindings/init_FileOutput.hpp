
#ifndef __init_FileOutput_HPP__
#define __init_FileOutput_HPP__

namespace sflight {

namespace xml  { class Node; }
namespace mdls { class FileOutput; }
namespace xml_bindings {
void init_FileOutput(sflight::xml::Node*, sflight::mdls::FileOutput*);
}

}

#endif
