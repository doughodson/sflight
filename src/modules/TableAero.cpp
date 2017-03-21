
#include "modules/TableAero.hpp"

#include "modules/Atmosphere.hpp"

#include "FDMGlobals.hpp"
#include "UnitConvert.hpp"
#include "WindAxis.hpp"
#include "Vector3.hpp"
#include "xml/Node.hpp"
#include "xml/node_utils.hpp"
#include "Table3D.hpp"
#include "Table2D.hpp"

#include <iostream>

namespace sf
{

TableAero::TableAero(FDMGlobals *globals, double frameRate) : FDMModule(globals, frameRate)
{
}

void TableAero::update(double timestep)
{
   if (globals == nullptr)
      return;

   double cl = liftTable->interp(globals->mach, globals->alt, globals->alpha);
   double cd = dragTable->interp(globals->mach, globals->alt, cl);

   double qbarS = 0.5 * globals->vInf * globals->vInf * globals->rho * wingArea;

   WindAxis::windToBody(globals->aeroForce, globals->alpha, globals->beta, cl * qbarS, cd * qbarS, 0);

   globals->fuelflow = thrustTable->interp(globals->mach, globals->alt, globals->throttle);
   globals->fuel = globals->fuel - globals->fuelflow * timestep;
   globals->mass = globals->mass - globals->fuelflow * timestep;

   double thrust = thrustTable->interp(globals->mach, globals->alt, globals->throttle);
   globals->thrust.set1(thrust * cos(thrustAngle));
   globals->thrust.set2(0);
   globals->thrust.set3(-thrust * sin(thrustAngle));
}

void TableAero::initialize(Node *node)
{

   Node *tmp = node->getChild("Design");
   if (tmp == 0)
      return;

   wingSpan = UnitConvert::toMeters(getDouble(tmp, "WingSpan", 6.0));
   wingArea = UnitConvert::toSqMeters(getDouble(tmp, "WingArea", 6.0));
   thrustAngle = UnitConvert::toRads(getDouble(tmp, "ThrustAngle", 0.0));

   Node *thrustNode = tmp->getChild("ThrustTable");
   Node *ffNode = tmp->getChild("FuelFlowTable");
   Node *liftNode = tmp->getChild("LiftTable");
   Node *dragNode = tmp->getChild("DragTable");

   if (thrustNode != 0)
   {

      std::vector<Node *> tables = thrustNode->getChildren("Table");
      int numpages = tables.size();
      double *throttleVals = new double[numpages];
      this->thrustTable = new Table3D(numpages, throttleVals);

      for (int i = 0; i < numpages; i++)
      {

         Node *tablenode = tables[i];

         throttleVals[i] = getDouble(tables[i], "Throttle", 0);

         std::string valstr = get(tablenode, "AltVals", "");
         std::vector<std::string> splits = splitString(valstr, ',');
         int numAltVals = splits.size();

         double *altvals = new double[numAltVals];
         for (int j = 0; j < numAltVals; j++)
         {
            altvals[j] = UnitConvert::toMeters(atof(splits[j].c_str()));
         }

         valstr = get(tablenode, "MachVals", "");
         splits = splitString(valstr, ',');
         int numMachVals = splits.size();

         double *machvals = new double[numMachVals];
         for (int j = 0; j < numMachVals; j++)
         {
            machvals[j] = atof(splits[j].c_str());
         }

         Table2D *table = new Table2D(numAltVals, numMachVals, machvals, altvals);
         table->setData(get(tablenode, "Data", ""));
         thrustTable->setPage(i, table);
      }
      // convert from lbs to Newtons
      thrustTable->multiply(UnitConvert::toNewtons(1));

      thrustTable->print();
   }

   if (ffNode != 0)
   {

      std::vector<Node *> tables = ffNode->getChildren("Table");
      int numpages = tables.size();
      double *throttleVals = new double[numpages];
      this->fuelflowTable = new Table3D(numpages, throttleVals);

      for (int i = 0; i < numpages; i++)
      {

         Node *tablenode = tables[i];

         throttleVals[i] = getDouble(tables[i], "Throttle", 0);

         std::string valstr = get(tablenode, "AltVals", "");
         std::vector<std::string> splits = splitString(valstr, ',');
         int numAltVals = splits.size();

         double *altvals = new double[numAltVals];
         for (int j = 0; j < numAltVals; j++)
         {
            altvals[j] = UnitConvert::toMeters(atof(splits[j].c_str()));
         }

         valstr = get(tablenode, "MachVals", "");
         splits = splitString(valstr, ',');
         int numMachVals = splits.size();

         double *machvals = new double[numMachVals];
         for (int j = 0; j < numMachVals; j++)
         {
            machvals[j] = atof(splits[j].c_str());
         }

         Table2D *table = new Table2D(numAltVals, numMachVals, altvals, machvals);
         table->setData(get(tablenode, "Data", ""));
         fuelflowTable->setPage(i, table);
      }
      //convert from lbs/sec to kilos/sec
      fuelflowTable->multiply(UnitConvert::toKilos(1));
   }

   if (liftNode != 0)
   {

      std::vector<Node *> tables = liftNode->getChildren("Table");
      int numpages = tables.size();
      double *machVals = new double[numpages];
      this->liftTable = new Table3D(numpages, machVals);

      for (int i = 0; i < numpages; i++)
      {
         Node *tablenode = tables[i];

         machVals[i] = getDouble(tables[i], "Mach", 0);

         std::string valstr = get(tablenode, "AltVals", "");
         std::vector<std::string> splits = splitString(valstr, ',');
         int numAltVals = splits.size();

         double *altvals = new double[numAltVals];
         for (int j = 0; j < numAltVals; j++)
         {
            altvals[j] = UnitConvert::toMeters(atof(splits[j].c_str()));
         }

         valstr = get(tablenode, "AlphaVals", "");
         splits = splitString(valstr, ',');
         int numAlphaVals = splits.size();

         double* alphavals = new double[numAlphaVals];
         for (int j = 0; j < numAlphaVals; j++)
         {
            alphavals[j] = UnitConvert::toRads(atof(splits[j].c_str()));
         }

         Table2D *table = new Table2D(numAltVals, numAlphaVals, altvals, alphavals);
         table->setData(get(tablenode, "Data", ""));
         liftTable->setPage(i, table);
      }
   }

   if (dragNode != 0)
   {

      std::vector<Node *> tables = dragNode->getChildren("Table");
      int numpages = tables.size();
      double *machVals = new double[numpages];
      this->dragTable = new Table3D(numpages, machVals);

      for (int i = 0; i < numpages; i++)
      {

         Node *tablenode = tables[i];

         machVals[i] = getDouble(tables[i], "Mach", 0);

         std::string valstr = get(tablenode, "AltVals", "");
         std::vector<std::string> splits = splitString(valstr, ',');
         int numAltVals = splits.size();

         double *altvals = new double[numAltVals];
         for (int j = 0; j < numAltVals; j++)
         {
            altvals[j] = UnitConvert::toMeters(atof(splits[j].c_str()));
         }

         valstr = get(tablenode, "CLVals", "");
         splits = splitString(valstr, ',');
         int numAlphaVals = splits.size();

         double *alphavals = new double[numAlphaVals];
         for (int j = 0; j < numAlphaVals; j++)
         {
            alphavals[j] = std::atof(splits[j].c_str());
         }

         Table2D *table = new Table2D(numAltVals, numAlphaVals, altvals, alphavals);
         table->setData(get(tablenode, "Data", ""));
         dragTable->setPage(i, table);
      }
   }
}
}
