
#ifndef __sflt_fdm_TableAero_H__
#define __sflt_fdm_TableAero_H__

#include "sflt/fdm/modules/FDMModule.hpp"

namespace sflt {
namespace xml {
class Node;
}
namespace fdm {
class Table3D;
class FDMGlobals;

//------------------------------------------------------------------------------
// Class: Quaternion
//------------------------------------------------------------------------------
class TableAero : public virtual FDMModule
{
 public:
   TableAero(FDMGlobals* globals, double frameRate);

   // module interface
   virtual void initialize(xml::Node* node) override;
   virtual void update(const double timestep) override;

   // void createCoefs( double pitch, double u, double vz, double thrust,
   // double& alpha, double& cl, double& cd );

 protected:
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
