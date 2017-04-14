
#ifndef __sflight_mdls_TableAero_H__
#define __sflight_mdls_TableAero_H__

#include "sflight/mdls/modules/Module.hpp"

namespace sflight {
namespace xml {
class Node;
}
namespace mdls {
class Table3D;
class Player;

//------------------------------------------------------------------------------
// Class: Quaternion
//------------------------------------------------------------------------------
class TableAero : public Module
{
 public:
   TableAero(Player*, const double frameRate);

   // module interface
   virtual void initialize(xml::Node*) override;
   virtual void update(const double timestep) override;

   // void createCoefs( double pitch, double u, double vz, double thrust,
   // double& alpha, double& cl, double& cd );

 private:
   double wingSpan{};
   double wingArea{};
   double thrustAngle{};

   Table3D* liftTable{};
   Table3D* dragTable{};
   Table3D* thrustTable{};
   Table3D* fuelflowTable{};

   double a1{};
   double a2{};
   double b1{};
   double b2{};

   double stallCL{};
   double wingEffects{};
};
}
}

#endif