
#include "sflight/mdls/AutoPilotCmds.hpp"
#include "sflight/mdls/UnitConvert.hpp"

namespace sflight {
namespace mdls {

void AutoPilotCmds :: setCmdHeading(const double radHeading)
{
   hdg = UnitConvert :: wrapHeading(radHeading, true);
}

}
}
