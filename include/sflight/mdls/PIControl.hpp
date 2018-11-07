
#ifndef __sflight_mdls_PIControl_HPP__
#define __sflight_mdls_PIControl_HPP__

namespace sflight {
namespace mdls {

//------------------------------------------------------------------------------
// Class: PIControl
// Description: This was taken from a Flightgear help page as a simple PIDControl
//              algorithm. The original source is not cited. It is identical to
//              the PID controller with differential control removed
//------------------------------------------------------------------------------
class PIControl
{
 public:
   PIControl() = delete;
   PIControl(double minVal, double maxVal, double p, double i);
   virtual ~PIControl() = default;

   virtual double getMax();
   virtual void setMax(double maxVal);

   virtual double getMin();
   virtual void setMin(double minVal);

   virtual double getP();
   virtual void setP(double p);

   virtual double getI();
   virtual void setI(double i);

   virtual double getOutput(double timestep, double desired_pt, double current_pt, double current_output);

 protected:
   double kp {};
   double alpha {0.1}, beta {1.0}, gamma {};
   double ts {}, ti {}, td {}, tf {};
   double en {};
   double rn {}, yn {};
   double max {}, min {};
   double du {}, u {};

   double ep0 {}, ep1 {};
};

}
}

#endif
