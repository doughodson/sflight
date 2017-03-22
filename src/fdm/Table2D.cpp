
#include "sf/fdm/Table2D.hpp"

#include "sf/xml/node_utils.hpp"

#include <iostream>
#include <string>

namespace sf {
namespace fdm {

Table2D::Table2D(const int numRows, const int numCols, double rowVals[],
                 double colVals[])
{
   this->rowVals = rowVals;
   this->colVals = colVals;
   this->numRows = numRows;
   this->numCols = numCols;

   // double data[numRows][numCols];
   data = new double*[numRows];

   for (int i = 0; i < numRows; i++) {
      data[i] = new double[numCols];
      for (int j = 0; j < numCols; j++) {
         data[i][j] = 0;
      }
   }
}

Table2D::~Table2D()
{
   for (int i = 0; i < numRows; i++) {
      delete[] data[i];
   }
   delete[] data;
   delete[] rowVals;
   delete[] colVals;
}

void Table2D::setData(std::string data)
{
   std::vector<std::string> v = xml::splitString(data, ',');

   if (v.size() != numRows * numCols)
      return;

   for (int i = 0; i < numRows; i++) {
      double* rowdata = new double[numCols];
      for (int j = 0; j < numCols; j++) {
         rowdata[j] = atof(v[i * numCols + j].c_str());
      }
      setRowData(i, rowdata);
   }
}

double** Table2D::getData() { return this->data; }

double* Table2D::getColVals() { return colVals; }

double* Table2D::getRowVals() { return rowVals; }

int Table2D::getNumCols() { return numCols; }

int Table2D::getNumRows() { return numRows; }

void Table2D::setRowData(int row, double rowdata[])
{
   if (row < numRows) {
      data[row] = rowdata;
   }
}

void Table2D::set(int row, int col, double val) { data[row][col] = val; }

double Table2D::get(int row, int col) { return data[row][col]; }

void Table2D::multiply(double val)
{
   for (int i = 0; i < numRows; i++) {
      for (int j = 0; j < numCols; j++) {
         data[i][j] = data[i][j] * val;
      }
   }
}

double Table2D::interp(double rowVal, double colVal)
{
   int lowrow{};
   int highrow{};
   double rowweight{};

   int lowcol{};
   int highcol{};
   double colweight{};

   for (int i = 1; i < numRows; i++) {
      lowrow = i - 1;
      highrow = i;
      if (rowVals[i] > rowVal) {
         break;
      }
   }
   if (numRows > 1)
      rowweight =
          (rowVal - rowVals[lowrow]) / (rowVals[highrow] - rowVals[lowrow]);

   for (int i = 1; i < numCols; i++) {
      lowcol = i - 1;
      highcol = i;
      if (colVals[i] > colVal) {
         break;
      }
   }
   if (numCols > 1)
      colweight =
          (colVal - colVals[lowcol]) / (colVals[highcol] - colVals[lowcol]);

   const double firstRow =
       data[lowrow][lowcol] +
       (data[lowrow][highcol] - data[lowrow][lowcol]) * colweight;
   const double secRow =
       data[highrow][lowcol] +
       (data[highrow][highcol] - data[highrow][lowcol]) * colweight;
   return firstRow + (secRow - firstRow) * rowweight;
}

void Table2D::print()
{
   std::cout << "row vals: [";
   for (int i = 0; i < numRows; i++) {
      std::cout << rowVals[i] << ", ";
   }
   std::cout << " ]" << std::endl << "col vals: [";
   for (int i = 0; i < numCols; i++) {
      std::cout << colVals[i] << ", ";
   }
   std::cout << "]" << std::endl;
   for (int i = 0; i < numRows; i++) {
      std::cout << "[ ";
      for (int j = 0; j < numCols; j++) {
         std::cout << data[i][j] << ", ";
      }
      std::cout << "]" << std::endl;
   }
}

// string toString() {
//
//
//        //for(int k=0; k<numPages; k++) {
//            //buf.append("Page: " + k + "\n");
//            for (int i=0; i<numRows; i++) {
//                buf.append("[ ");
//                for (int j=0; j < numCols; j++) {
//                    buf.append(data[i][j] + ", ");
//                }
//                buf.append(" ]\n");
//            }
//        //}
//        return buf.toString();
//    }
}
}
