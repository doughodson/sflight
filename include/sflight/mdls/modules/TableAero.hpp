
#ifndef __sflight_mdls_TableAero_H__
#define __sflight_mdls_TableAero_H__

#include "sflight/mdls/modules/Module.hpp"

#include "sflight/xml_bindings/init_TableAero.hpp"
#include "sflight/xml/Node.hpp"

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
   virtual void update(const double timestep) override;

   // void createCoefs( double pitch, double u, double vz, double thrust,
   // double& alpha, double& cl, double& cd );

   friend void xml_bindings::init_TableAero(xml::Node*, TableAero*);

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
