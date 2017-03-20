
#ifndef PID_H
#define PID_H

namespace sf
{
class PID
{

public :

    PID();
    PID(double minVal, double maxVal, double kp);
    ~PID();

    void setIDTimes( double ti, double td);

    void setGain( double p);

    double getP();
    double getI();
    double getD();

    double getOutput(double timestep, double desired_pt, double current_pt, double current_output);

    double kp;
    double alpha, beta, gamma;
    double ts, ti, td, tf;
    double en;
    double rn, yn;
    double max, min;
    double du, u;

    double ep[2];
    double ed[3];
    double edf[4];


};

}

#endif
