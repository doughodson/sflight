
#include "AutoPilotCmds.hpp"
#include "UnitConvert.hpp"

namespace sf {

void AutoPilotCmds :: setCmdHeading(const double radHeading)
{
    hdg = UnitConvert :: wrapHeading(radHeading, true);
}

}
