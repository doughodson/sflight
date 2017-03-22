
#include "sf/fdm/Table3D.hpp"

#include <iostream>

namespace sf {
namespace fdm {

Table3D::Table3D(const int numPages, double pageVals[])
    : pageVals(pageVals), numPages(numPages)
{
   data = new Table2D*[numPages];
}

Table3D::~Table3D()
{
   if (data) {
      delete[] data;
   }
   delete[] pageVals;
}

int Table3D::getNumPages() { return this->numPages; }

void Table3D::multiply(const double val)
{
   for (int i = 0; i < numPages; i++) {
      data[i]->multiply(val);
   }
}

void Table3D::setPage(const int page, Table2D* table)
{
   if (page < numPages) {
      data[page] = table;
   }
}

Table2D* Table3D::getPage(const int page)
{
   if (page < numPages) {
      return data[page];
   }
}

double Table3D::interp(const double pageVal, const double rowVal,
                       const double colVal)
{
   int lowpage = 0;
   int highpage = 0;
   double pageweight = 0;

   for (int i = 1; i < numPages; i++) {
      lowpage = i - 1;
      highpage = i;
      if (pageVals[i] > pageVal) {
         break;
      }
   }
   if (numPages > 1)
      pageweight = (pageVal - pageVals[lowpage]) /
                   (pageVals[highpage] - pageVals[lowpage]);

   const double firstpage = data[lowpage]->interp(rowVal, colVal);
   const double secpage = data[highpage]->interp(rowVal, colVal);

   return firstpage + (secpage - firstpage) * pageweight;
}

void Table3D::print() const
{
   for (int i = 0; i < numPages; i++) {
      std::cout << "page: " << i << ", val: " << pageVals[i] << std::endl;
      data[i]->print();
   }
}
}
}
