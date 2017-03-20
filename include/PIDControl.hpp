// this is taken from a Flightgear help page as a simple PIDControl algorithm. The original source is not cited.
// it is identical to the PID controller with differential control removed

#ifndef PIDCONTROL_H
#define PIDCONTROL_H

#include "PIControl.hpp"

namespace sf {

class PIDControl : public PIControl {

    public:
   
    PIDControl();
    PIDControl(double minVal, double maxVal, double p, double i, double d);
    ~PIDControl();


    virtual double getD();
    virtual void setD(double d);

    virtual double getOutput(double timestep, double desired_pt, double current_pt, double current_output);

    protected:

    double edf0, edf1, edf2;
    double tstf;
    double ed;


};

}

#endif
