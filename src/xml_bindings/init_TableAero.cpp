
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
#include <iostream>

namespace sflight {
namespace xml_bindings {

void init_TableAero(xml::Node* node, mdls::TableAero* tblAero)
{
   std::cout << std::endl;
   std::cout << "-------------------------" << std::endl;
   std::cout << "Module: TableAero"         << std::endl;
   std::cout << "-------------------------" << std::endl;

   xml::Node* tmp{node->getChild("Design")};
   if (!tmp) { return; }

   tblAero->wingSpan = mdls::UnitConvert::toMeters(xml::getDouble(tmp, "WingSpan", 6.0));
   tblAero->wingArea = mdls::UnitConvert::toSqMeters(xml::getDouble(tmp, "WingArea", 6.0));
   tblAero->thrustAngle = mdls::UnitConvert::toRads(xml::getDouble(tmp, "ThrustAngle", 0.0));

   xml::Node* thrustNode{tmp->getChild("ThrustTable")};
   xml::Node* ffNode{tmp->getChild("FuelFlowTable")};
   xml::Node* liftNode{tmp->getChild("LiftTable")};
   xml::Node* dragNode{tmp->getChild("DragTable")};

   if (thrustNode) {

      std::vector<xml::Node*> tables{thrustNode->getChildren("Table")};
      const std::size_t numpages{tables.size()};
      double* throttleVals{new double[numpages]};
      tblAero->thrustTable = new mdls::Table3D(numpages, throttleVals);

      for (std::size_t i = 0; i < numpages; i++) {

         xml::Node* tablenode{tables[i]};

         throttleVals[i] = xml::getDouble(tables[i], "Throttle", 0.0);

         std::string valstr{xml::getString(tablenode, "AltVals", "")};
         std::vector<std::string> splits{xml::splitString(valstr, ',')};
         const std::size_t numAltVals{splits.size()};

         double* altvals{new double[numAltVals]};
         for (std::size_t j = 0; j < numAltVals; j++) {
            altvals[j] = mdls::UnitConvert::toMeters(std::atof(splits[j].c_str()));
         }

         valstr = xml::getString(tablenode, "MachVals", "");
         splits = xml::splitString(valstr, ',');
         std::size_t numMachVals{splits.size()};

         double* machvals{new double[numMachVals]};
         for (std::size_t j = 0; j < numMachVals; j++) {
            machvals[j] = std::atof(splits[j].c_str());
         }

         mdls::Table2D* table{new mdls::Table2D(numAltVals, numMachVals, machvals, altvals)};
         const std::string data{xml::getString(tablenode, "Data", "")};
         table->setData(data);
         tblAero->thrustTable->setPage(i, table);
      }
      // convert from lbs to Newtons
      tblAero->thrustTable->multiply(mdls::UnitConvert::toNewtons(1));

      tblAero->thrustTable->print();
   }

   if (ffNode) {

      std::vector<xml::Node*> tables{ffNode->getChildren("Table")};
      const std::size_t numpages{tables.size()};
      double* throttleVals{new double[numpages]};
      tblAero->fuelflowTable = new mdls::Table3D(numpages, throttleVals);

      for (std::size_t i = 0; i < numpages; i++) {

         xml::Node* tablenode{tables[i]};

         throttleVals[i] = xml::getDouble(tables[i], "Throttle", 0.0);

         std::string valstr{getString(tablenode, "AltVals", "")};
         std::vector<std::string> splits{xml::splitString(valstr, ',')};
         std::size_t numAltVals{splits.size()};

         double* altvals{new double[numAltVals]};
         for (std::size_t j = 0; j < numAltVals; j++) {
            altvals[j] = mdls::UnitConvert::toMeters(std::atof(splits[j].c_str()));
         }

         valstr = xml::getString(tablenode, "MachVals", "");
         splits = xml::splitString(valstr, ',');
         const std::size_t numMachVals{splits.size()};

         double* machvals{new double[numMachVals]};
         for (std::size_t j = 0; j < numMachVals; j++) {
            machvals[j] = std::atof(splits[j].c_str());
         }

         mdls::Table2D* table{new mdls::Table2D(numAltVals, numMachVals, altvals, machvals)};
         table->setData(xml::getString(tablenode, "Data", ""));
         tblAero->fuelflowTable->setPage(i, table);
      }
      // convert from lbs/sec to kilos/sec
      tblAero->fuelflowTable->multiply(mdls::UnitConvert::toKilos(1));
   }

   if (liftNode != nullptr) {

      std::vector<xml::Node*> tables{liftNode->getChildren("Table")};
      const std::size_t numpages{tables.size()};
      double* machVals{new double[numpages]};
      tblAero->liftTable = new mdls::Table3D(numpages, machVals);

      for (std::size_t i = 0; i < numpages; i++) {
         xml::Node* tablenode{tables[i]};

         machVals[i] = xml::getDouble(tables[i], "Mach", 0.0);

         std::string valstr{xml::getString(tablenode, "AltVals", "")};
         std::vector<std::string> splits{xml::splitString(valstr, ',')};
         const std::size_t numAltVals{splits.size()};

         double* altvals{new double[numAltVals]};
         for (std::size_t j = 0; j < numAltVals; j++) {
            altvals[j] = mdls::UnitConvert::toMeters(std::atof(splits[j].c_str()));
         }

         valstr = xml::getString(tablenode, "AlphaVals", "");
         splits = xml::splitString(valstr, ',');
         const std::size_t numAlphaVals{splits.size()};

         double* alphavals{new double[numAlphaVals]};
         for (std::size_t j = 0; j < numAlphaVals; j++) {
            alphavals[j] = mdls::UnitConvert::toRads(std::atof(splits[j].c_str()));
         }

         mdls::Table2D* table{new mdls::Table2D(numAltVals, numAlphaVals, altvals, alphavals)};
         table->setData(getString(tablenode, "Data", ""));
         tblAero->liftTable->setPage(i, table);
      }
   }

   if (dragNode != nullptr) {
      std::vector<xml::Node*> tables{dragNode->getChildren("Table")};
      const std::size_t numpages{tables.size()};
      double* machVals{new double[numpages]};
      tblAero->dragTable = new mdls::Table3D(numpages, machVals);

      for (std::size_t i = 0; i < numpages; i++) {

         xml::Node* tablenode{tables[i]};

         machVals[i] = xml::getDouble(tables[i], "Mach", 0.0);

         std::string valstr{xml::getString(tablenode, "AltVals", "")};
         std::vector<std::string> splits{xml::splitString(valstr, ',')};
         const std::size_t numAltVals{splits.size()};

         double* altvals{new double[numAltVals]};
         for (std::size_t j = 0; j < numAltVals; j++) {
            altvals[j] = mdls::UnitConvert::toMeters(std::atof(splits[j].c_str()));
         }

         valstr = xml::getString(tablenode, "CLVals", "");
         splits = xml::splitString(valstr, ',');
         const std::size_t numAlphaVals{splits.size()};

         double* alphavals{new double[numAlphaVals]};
         for (std::size_t j = 0; j < numAlphaVals; j++) {
            alphavals[j] = std::atof(splits[j].c_str());
         }

         mdls::Table2D* table{new mdls::Table2D(numAltVals, numAlphaVals, altvals, alphavals)};
         table->setData(xml::getString(tablenode, "Data", ""));
         tblAero->dragTable->setPage(i, table);
      }
   }

   std::cout << "-------------------------" << std::endl;
}
}
}
