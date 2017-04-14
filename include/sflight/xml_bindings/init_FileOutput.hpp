
#ifndef __init_FileOutput_H__
#define __init_FileOutput_H__

namespace sflight {
namespace xml {
class Node;
}
namespace mdls {
class FileOutput;
}
namespace xml_bindings {

void init_FileOutput(sflight::xml::Node*, sflight::mdls::FileOutput*);
}
}

#endif
