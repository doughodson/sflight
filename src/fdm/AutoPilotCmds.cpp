
#include "sflt/fdm/AutoPilotCmds.hpp"
#include "sflt/fdm/UnitConvert.hpp"

namespace sflt {
namespace fdm {

void AutoPilotCmds :: setCmdHeading(const double radHeading)
{
   hdg = UnitConvert :: wrapHeading(radHeading, true);
}

}
}
