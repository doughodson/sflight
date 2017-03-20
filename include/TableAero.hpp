/** sets up a simple aero lookup table system */

#ifndef TABLEAERO_H
#define TABLEAERO_H

#include "FDMModule.hpp"



namespace SimpleFlight
{
    
class Table3D;
class FDMGlobals;

class TableAero : public virtual FDMModule{

    public:

    TableAero( FDMGlobals *globals, double frameRate);

    // functions from FDMModule
    void initialize(Node* node);
    void update( double timestep );

    //void createCoefs( double pitch, double u, double vz, double thrust, double& alpha, double& cl, double& cd );

    protected:

    double wingSpan;
    double wingArea;
    double thrustAngle;

    Table3D *liftTable;
    Table3D *dragTable;
    Table3D *thrustTable;
    Table3D *fuelflowTable;

    double a1;
    double a2;
    double b1;
    double b2;

	double stallCL;
	double wingEffects;


};

}//namespace SimpleFlight
#endif
