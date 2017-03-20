

#include "AutoPilotCmds.hpp"
#include "UnitConvert.hpp"
#include <math.h>

namespace SimpleFlight
{

AutoPilotCmds :: AutoPilotCmds()
{
   this->apOn = false;
   this->atOn = false;
   this->hdgHoldOn = false;
   this->altHoldOn = false;
   this->vsHoldOn = false;
   this->orbitHoldOn = false;
   this->useMach = false;
   this->levelOn = false;
   

   this->hdg = 0;
   this->alt = 0;
   this->vel = 0;
   this->vs = 0;
   this->sideslip = 0;

}


AutoPilotCmds :: ~AutoPilotCmds()
{}

void AutoPilotCmds :: setAutoPilotOn(bool onOff)
{
    this->apOn = onOff;
}


bool AutoPilotCmds :: isAutoPilotOn()
{
    return this->apOn;
}

void AutoPilotCmds :: setAutoThrottleOn(bool onOff)
{
    this->atOn = onOff;
}

bool AutoPilotCmds :: isAutoThrottleOn()
{
    return this->atOn;
}

void AutoPilotCmds :: setCmdHeading(double radHeading)
{
    this->hdg = UnitConvert :: wrapHeading(radHeading, true);
}

double AutoPilotCmds :: getCmdHeading()
{
    return this->hdg;
}

void AutoPilotCmds :: setCmdAltitude(double metersMSLAlt)
{
    this->alt = metersMSLAlt;
}

void AutoPilotCmds::setMaxPitchUp(double radPitch)
{
    this->maxPitch = radPitch;
}

double AutoPilotCmds::getMaxPitchUp()
{
    return maxPitch;
}

void AutoPilotCmds::setMaxPitchDown(double radPitch)
{
    this->minPitch = radPitch;
}

double AutoPilotCmds::getMaxPitchDown()
{
    return minPitch;
}

void AutoPilotCmds::setMaxBank(double radBank)
{
    this->maxBank = radBank;
}

double AutoPilotCmds::getMaxBank()
{
    return maxBank;
}

void AutoPilotCmds :: setMaxVS(double mpsVs)
{
    this->maxVS = mpsVs;
}

double AutoPilotCmds::getMaxVS()
{
    return maxVS;
}

double AutoPilotCmds :: getCmdAltitude()
{
    return this->alt;
}

void AutoPilotCmds :: setCmdVertSpeed(double metersPerSec)
{
    this->vs = metersPerSec;
}

double AutoPilotCmds :: getCmdVertSpeed()
{
    return this->vs;
}

void AutoPilotCmds :: setCmdSpeed( double metersPerSec )
{
    this->vel = metersPerSec;
}

double AutoPilotCmds :: getCmdSpeed()
{
    return this->vel;
}

bool AutoPilotCmds :: isAltHoldOn()
{
    return this->altHoldOn;
}

void AutoPilotCmds :: setAltHoldOn(bool altHoldOn)
{
    this->altHoldOn = altHoldOn;
}

bool AutoPilotCmds :: isVsHoldOn()
{
    return this->vsHoldOn;
}

void AutoPilotCmds :: setVsHoldOn(bool vsHoldOn)
{
    this->vsHoldOn = vsHoldOn;
}

bool AutoPilotCmds :: isHdgHoldOn()
{
    return this->hdgHoldOn;
}

void AutoPilotCmds :: setHdgHoldOn(bool hdgHoldOn)
{
    this->hdgHoldOn = hdgHoldOn;
}

bool AutoPilotCmds :: isOrbitHoldOn()
{
    return this->orbitHoldOn;
}

void AutoPilotCmds :: setOrbitHoldOn(bool orbitHoldOn)
{
    this->orbitHoldOn = orbitHoldOn;
}

bool AutoPilotCmds :: isLevelOn()
{
    return this->levelOn;
}

void AutoPilotCmds :: setLevelOn(bool levelOn)
{
    this->levelOn = levelOn;
}

void AutoPilotCmds :: setCmdMach(double mach) {
	this->mach = mach;
}

double AutoPilotCmds :: getCmdMach() {
	return this->mach;
}

void AutoPilotCmds :: setCmdSideSlip(double mpsSideslip) {
	this->sideslip = mpsSideslip;
}

double AutoPilotCmds :: getCmdSideSlip() {
	return this->sideslip;
}

bool AutoPilotCmds :: isUsingMach() {
	return this->useMach;
}

void AutoPilotCmds :: setUseMach(bool useMach) {
	this->useMach = useMach;
}


} //namespace SimpleFlight
