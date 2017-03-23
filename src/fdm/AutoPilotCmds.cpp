
#include "sflight/fdm/AutoPilotCmds.hpp"
#include "sflight/fdm/UnitConvert.hpp"

namespace sflight {
namespace fdm {

void AutoPilotCmds :: setCmdHeading(const double radHeading)
{
   hdg = UnitConvert :: wrapHeading(radHeading, true);
}

}
}
