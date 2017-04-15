
#include "sflight/xml_bindings/init_TableAero.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/modules/TableAero.hpp"

#include "sflight/mdls/UnitConvert.hpp"
#include "sflight/mdls/constants.hpp"
#include "sflight/mdls/Table2D.hpp"
#include "sflight/mdls/Table3D.hpp"

#include <cmath>

namespace sflight {
namespace xml_bindings {

void init_TableAero(xml::Node* node, mdls::TableAero* tblAero)
{
   xml::Node* tmp = node->getChild("Design");
   if (tmp == 0)
      return;

   tblAero->wingSpan = mdls::UnitConvert::toMeters(xml::getDouble(tmp, "WingSpan", 6.0));
   tblAero->wingArea = mdls::UnitConvert::toSqMeters(xml::getDouble(tmp, "WingArea", 6.0));
   tblAero->thrustAngle = mdls::UnitConvert::toRads(xml::getDouble(tmp, "ThrustAngle", 0.0));

   xml::Node* thrustNode = tmp->getChild("ThrustTable");
   xml::Node* ffNode = tmp->getChild("FuelFlowTable");
   xml::Node* liftNode = tmp->getChild("LiftTable");
   xml::Node* dragNode = tmp->getChild("DragTable");

   if (thrustNode != nullptr) {

      std::vector<xml::Node*> tables = thrustNode->getChildren("Table");
      const int numpages = tables.size();
      double* throttleVals = new double[numpages];
      tblAero->thrustTable = new mdls::Table3D(numpages, throttleVals);

      for (int i = 0; i < numpages; i++) {

         xml::Node* tablenode = tables[i];

         throttleVals[i] = xml::getDouble(tables[i], "Throttle", 0.0);

         std::string valstr = xml::get(tablenode, "AltVals", "");
         std::vector<std::string> splits = xml::splitString(valstr, ',');
         int numAltVals = splits.size();

         double* altvals = new double[numAltVals];
         for (int j = 0; j < numAltVals; j++) {
            altvals[j] = mdls::UnitConvert::toMeters(std::atof(splits[j].c_str()));
         }

         valstr = xml::get(tablenode, "MachVals", "");
         splits = xml::splitString(valstr, ',');
         int numMachVals = splits.size();

         double* machvals = new double[numMachVals];
         for (int j = 0; j < numMachVals; j++) {
            machvals[j] = std::atof(splits[j].c_str());
         }

         mdls::Table2D* table =
             new mdls::Table2D(numAltVals, numMachVals, machvals, altvals);
         table->setData(xml::get(tablenode, "Data", ""));
         tblAero->thrustTable->setPage(i, table);
      }
      // convert from lbs to Newtons
      tblAero->thrustTable->multiply(mdls::UnitConvert::toNewtons(1));

      tblAero->thrustTable->print();
   }

   if (ffNode != nullptr) {

      std::vector<xml::Node*> tables = ffNode->getChildren("Table");
      const int numpages = tables.size();
      double* throttleVals = new double[numpages];
      tblAero->fuelflowTable = new mdls::Table3D(numpages, throttleVals);

      for (int i = 0; i < numpages; i++) {

         xml::Node* tablenode = tables[i];

         throttleVals[i] = xml::getDouble(tables[i], "Throttle", 0.0);

         std::string valstr = get(tablenode, "AltVals", "");
         std::vector<std::string> splits = xml::splitString(valstr, ',');
         int numAltVals = splits.size();

         double* altvals = new double[numAltVals];
         for (int j = 0; j < numAltVals; j++) {
            altvals[j] = mdls::UnitConvert::toMeters(std::atof(splits[j].c_str()));
         }

         valstr = xml::get(tablenode, "MachVals", "");
         splits = xml::splitString(valstr, ',');
         int numMachVals = splits.size();

         double* machvals = new double[numMachVals];
         for (int j = 0; j < numMachVals; j++) {
            machvals[j] = std::atof(splits[j].c_str());
         }

         mdls::Table2D* table =
             new mdls::Table2D(numAltVals, numMachVals, altvals, machvals);
         table->setData(xml::get(tablenode, "Data", ""));
         tblAero->fuelflowTable->setPage(i, table);
      }
      // convert from lbs/sec to kilos/sec
      tblAero->fuelflowTable->multiply(mdls::UnitConvert::toKilos(1));
   }

   if (liftNode != nullptr) {

      std::vector<xml::Node*> tables = liftNode->getChildren("Table");
      const int numpages = tables.size();
      double* machVals = new double[numpages];
      tblAero->liftTable = new mdls::Table3D(numpages, machVals);

      for (int i = 0; i < numpages; i++) {
         xml::Node* tablenode = tables[i];

         machVals[i] = xml::getDouble(tables[i], "Mach", 0.0);

         std::string valstr = xml::get(tablenode, "AltVals", "");
         std::vector<std::string> splits = xml::splitString(valstr, ',');
         int numAltVals = splits.size();

         double* altvals = new double[numAltVals];
         for (int j = 0; j < numAltVals; j++) {
            altvals[j] = mdls::UnitConvert::toMeters(std::atof(splits[j].c_str()));
         }

         valstr = xml::get(tablenode, "AlphaVals", "");
         splits = xml::splitString(valstr, ',');
         int numAlphaVals = splits.size();

         double* alphavals = new double[numAlphaVals];
         for (int j = 0; j < numAlphaVals; j++) {
            alphavals[j] = mdls::UnitConvert::toRads(std::atof(splits[j].c_str()));
         }

         mdls::Table2D* table =
             new mdls::Table2D(numAltVals, numAlphaVals, altvals, alphavals);
         table->setData(get(tablenode, "Data", ""));
         tblAero->liftTable->setPage(i, table);
      }
   }

   if (dragNode != nullptr) {
      std::vector<xml::Node*> tables = dragNode->getChildren("Table");
      int numpages = tables.size();
      double* machVals = new double[numpages];
      tblAero->dragTable = new mdls::Table3D(numpages, machVals);

      for (int i = 0; i < numpages; i++) {

         xml::Node* tablenode = tables[i];

         machVals[i] = xml::getDouble(tables[i], "Mach", 0.0);

         std::string valstr = xml::get(tablenode, "AltVals", "");
         std::vector<std::string> splits = xml::splitString(valstr, ',');
         const int numAltVals = splits.size();

         double* altvals = new double[numAltVals];
         for (int j = 0; j < numAltVals; j++) {
            altvals[j] = mdls::UnitConvert::toMeters(std::atof(splits[j].c_str()));
         }

         valstr = xml::get(tablenode, "CLVals", "");
         splits = xml::splitString(valstr, ',');
         const int numAlphaVals = splits.size();

         double* alphavals = new double[numAlphaVals];
         for (int j = 0; j < numAlphaVals; j++) {
            alphavals[j] = std::atof(splits[j].c_str());
         }

         mdls::Table2D* table =
             new mdls::Table2D(numAltVals, numAlphaVals, altvals, alphavals);
         table->setData(xml::get(tablenode, "Data", ""));
         tblAero->dragTable->setPage(i, table);
      }
   }

}
}
}
