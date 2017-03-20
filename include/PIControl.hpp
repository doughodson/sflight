
// this is taken from a Flightgear help page as a simple PIDControl algorithm. The original source is not cited.
// it is identical to the PID controller with differential control removed

#ifndef PICONTROL_H
#define PICONTROL_H

namespace sf {

class PIControl {

    public:

    PIControl();
    PIControl(double minVal, double maxVal, double p, double i);
    ~PIControl();

    virtual double getMax();
    virtual void setMax( double maxVal);

    virtual double getMin();
    virtual void setMin( double minVal);

    virtual double getP();
    virtual void setP(double p);

    virtual double getI();
    virtual void setI(double i);

    virtual double getOutput(double timestep, double desired_pt, double current_pt, double current_output);


    protected:

    double kp;
    double alpha, beta, gamma;
    double ts, ti, td, tf;
    double en;
    double rn, yn;
    double max, min;
    double du, u;

    double ep0, ep1;

};

}

#endif
