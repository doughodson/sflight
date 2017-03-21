
#include "fdm/AutoPilotCmds.hpp"
#include "fdm/UnitConvert.hpp"

namespace sf {

void AutoPilotCmds :: setCmdHeading(const double radHeading)
{
    hdg = UnitConvert :: wrapHeading(radHeading, true);
}

}
