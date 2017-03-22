
#include "sf/fdm/modules/TableAero.hpp"

#include "sf/fdm/modules/Atmosphere.hpp"

#include "sf/xml/Node.hpp"
#include "sf/xml/node_utils.hpp"

#include "sf/fdm/FDMGlobals.hpp"
#include "sf/fdm/Table2D.hpp"
#include "sf/fdm/Table3D.hpp"
#include "sf/fdm/UnitConvert.hpp"
#include "sf/fdm/Vector3.hpp"
#include "sf/fdm/WindAxis.hpp"

#include <cmath>
#include <iostream>

namespace sf {
namespace fdm {

TableAero::TableAero(FDMGlobals* globals, double frameRate)
    : FDMModule(globals, frameRate)
{
}

void TableAero::update(const double timestep)
{
   if (globals == nullptr)
      return;

   double cl = liftTable->interp(globals->mach, globals->alt, globals->alpha);
   double cd = dragTable->interp(globals->mach, globals->alt, cl);

   double qbarS = 0.5 * globals->vInf * globals->vInf * globals->rho * wingArea;

   WindAxis::windToBody(globals->aeroForce, globals->alpha, globals->beta,
                        cl * qbarS, cd * qbarS, 0);

   globals->fuelflow =
       thrustTable->interp(globals->mach, globals->alt, globals->throttle);
   globals->fuel = globals->fuel - globals->fuelflow * timestep;
   globals->mass = globals->mass - globals->fuelflow * timestep;

   double thrust =
       thrustTable->interp(globals->mach, globals->alt, globals->throttle);
   globals->thrust.set1(thrust * cos(thrustAngle));
   globals->thrust.set2(0);
   globals->thrust.set3(-thrust * sin(thrustAngle));
}

void TableAero::initialize(xml::Node* node)
{

   xml::Node* tmp = node->getChild("Design");
   if (tmp == 0)
      return;

   wingSpan = UnitConvert::toMeters(xml::getDouble(tmp, "WingSpan", 6.0));
   wingArea = UnitConvert::toSqMeters(xml::getDouble(tmp, "WingArea", 6.0));
   thrustAngle = UnitConvert::toRads(xml::getDouble(tmp, "ThrustAngle", 0.0));

   xml::Node* thrustNode = tmp->getChild("ThrustTable");
   xml::Node* ffNode = tmp->getChild("FuelFlowTable");
   xml::Node* liftNode = tmp->getChild("LiftTable");
   xml::Node* dragNode = tmp->getChild("DragTable");

   if (thrustNode != nullptr) {

      std::vector<xml::Node*> tables = thrustNode->getChildren("Table");
      const int numpages = tables.size();
      double* throttleVals = new double[numpages];
      this->thrustTable = new Table3D(numpages, throttleVals);

      for (int i = 0; i < numpages; i++) {

         xml::Node* tablenode = tables[i];

         throttleVals[i] = xml::getDouble(tables[i], "Throttle", 0);

         std::string valstr = xml::get(tablenode, "AltVals", "");
         std::vector<std::string> splits = xml::splitString(valstr, ',');
         int numAltVals = splits.size();

         double* altvals = new double[numAltVals];
         for (int j = 0; j < numAltVals; j++) {
            altvals[j] = UnitConvert::toMeters(std::atof(splits[j].c_str()));
         }

         valstr = xml::get(tablenode, "MachVals", "");
         splits = xml::splitString(valstr, ',');
         int numMachVals = splits.size();

         double* machvals = new double[numMachVals];
         for (int j = 0; j < numMachVals; j++) {
            machvals[j] = std::atof(splits[j].c_str());
         }

         Table2D* table =
             new Table2D(numAltVals, numMachVals, machvals, altvals);
         table->setData(xml::get(tablenode, "Data", ""));
         thrustTable->setPage(i, table);
      }
      // convert from lbs to Newtons
      thrustTable->multiply(UnitConvert::toNewtons(1));

      thrustTable->print();
   }

   if (ffNode != nullptr) {

      std::vector<xml::Node*> tables = ffNode->getChildren("Table");
      int numpages = tables.size();
      double* throttleVals = new double[numpages];
      this->fuelflowTable = new Table3D(numpages, throttleVals);

      for (int i = 0; i < numpages; i++) {

         xml::Node* tablenode = tables[i];

         throttleVals[i] = getDouble(tables[i], "Throttle", 0);

         std::string valstr = get(tablenode, "AltVals", "");
         std::vector<std::string> splits = xml::splitString(valstr, ',');
         int numAltVals = splits.size();

         double* altvals = new double[numAltVals];
         for (int j = 0; j < numAltVals; j++) {
            altvals[j] = UnitConvert::toMeters(std::atof(splits[j].c_str()));
         }

         valstr = get(tablenode, "MachVals", "");
         splits = xml::splitString(valstr, ',');
         int numMachVals = splits.size();

         double* machvals = new double[numMachVals];
         for (int j = 0; j < numMachVals; j++) {
            machvals[j] = std::atof(splits[j].c_str());
         }

         Table2D* table =
             new Table2D(numAltVals, numMachVals, altvals, machvals);
         table->setData(xml::get(tablenode, "Data", ""));
         fuelflowTable->setPage(i, table);
      }
      // convert from lbs/sec to kilos/sec
      fuelflowTable->multiply(UnitConvert::toKilos(1));
   }

   if (liftNode != nullptr) {

      std::vector<xml::Node*> tables = liftNode->getChildren("Table");
      int numpages = tables.size();
      double* machVals = new double[numpages];
      this->liftTable = new Table3D(numpages, machVals);

      for (int i = 0; i < numpages; i++) {
         xml::Node* tablenode = tables[i];

         machVals[i] = getDouble(tables[i], "Mach", 0);

         std::string valstr = xml::get(tablenode, "AltVals", "");
         std::vector<std::string> splits = xml::splitString(valstr, ',');
         int numAltVals = splits.size();

         double* altvals = new double[numAltVals];
         for (int j = 0; j < numAltVals; j++) {
            altvals[j] = UnitConvert::toMeters(std::atof(splits[j].c_str()));
         }

         valstr = xml::get(tablenode, "AlphaVals", "");
         splits = xml::splitString(valstr, ',');
         int numAlphaVals = splits.size();

         double* alphavals = new double[numAlphaVals];
         for (int j = 0; j < numAlphaVals; j++) {
            alphavals[j] = UnitConvert::toRads(std::atof(splits[j].c_str()));
         }

         Table2D* table =
             new Table2D(numAltVals, numAlphaVals, altvals, alphavals);
         table->setData(get(tablenode, "Data", ""));
         liftTable->setPage(i, table);
      }
   }

   if (dragNode != nullptr) {
      std::vector<xml::Node*> tables = dragNode->getChildren("Table");
      int numpages = tables.size();
      double* machVals = new double[numpages];
      this->dragTable = new Table3D(numpages, machVals);

      for (int i = 0; i < numpages; i++) {

         xml::Node* tablenode = tables[i];

         machVals[i] = xml::getDouble(tables[i], "Mach", 0);

         std::string valstr = xml::get(tablenode, "AltVals", "");
         std::vector<std::string> splits = xml::splitString(valstr, ',');
         int numAltVals = splits.size();

         double* altvals = new double[numAltVals];
         for (int j = 0; j < numAltVals; j++) {
            altvals[j] = UnitConvert::toMeters(std::atof(splits[j].c_str()));
         }

         valstr = xml::get(tablenode, "CLVals", "");
         splits = xml::splitString(valstr, ',');
         int numAlphaVals = splits.size();

         double* alphavals = new double[numAlphaVals];
         for (int j = 0; j < numAlphaVals; j++) {
            alphavals[j] = std::atof(splits[j].c_str());
         }

         Table2D* table =
             new Table2D(numAltVals, numAlphaVals, altvals, alphavals);
         table->setData(xml::get(tablenode, "Data", ""));
         dragTable->setPage(i, table);
      }
   }
}
}
}
