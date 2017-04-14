
#include "sflight/mdls/modules/TableAero.hpp"

#include "sflight/mdls/modules/Atmosphere.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/Table2D.hpp"
#include "sflight/mdls/Table3D.hpp"
#include "sflight/mdls/UnitConvert.hpp"
#include "sflight/mdls/Vector3.hpp"
#include "sflight/mdls/WindAxis.hpp"

#include <cmath>
#include <iostream>

namespace sflight {
namespace mdls {

TableAero::TableAero(Player* player, const double frameRate)
    : Module(player, frameRate)
{
}

void TableAero::update(const double timestep)
{
   if (player == nullptr)
      return;

   double cl = liftTable->interp(player->mach, player->alt, player->alpha);
   double cd = dragTable->interp(player->mach, player->alt, cl);

   double qbarS = 0.5 * player->vInf * player->vInf * player->rho * wingArea;

   WindAxis::windToBody(player->aeroForce, player->alpha, player->beta,
                        cl * qbarS, cd * qbarS, 0);

   player->fuelflow =
       thrustTable->interp(player->mach, player->alt, player->throttle);
   player->fuel = player->fuel - player->fuelflow * timestep;
   player->mass = player->mass - player->fuelflow * timestep;

   double thrust =
       thrustTable->interp(player->mach, player->alt, player->throttle);
   player->thrust.set1(thrust * std::cos(thrustAngle));
   player->thrust.set2(0);
   player->thrust.set3(-thrust * std::sin(thrustAngle));
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

         throttleVals[i] = xml::getDouble(tables[i], "Throttle", 0);

         std::string valstr = get(tablenode, "AltVals", "");
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

         machVals[i] = xml::getDouble(tables[i], "Mach", 0);

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
