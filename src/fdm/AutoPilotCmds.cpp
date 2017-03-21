
#include "sf/fdm/AutoPilotCmds.hpp"
#include "sf/fdm/UnitConvert.hpp"

namespace sf {

void AutoPilotCmds :: setCmdHeading(const double radHeading)
{
    hdg = UnitConvert :: wrapHeading(radHeading, true);
}

}
