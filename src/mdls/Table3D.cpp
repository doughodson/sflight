
#include "sflight/mdls/Table3D.hpp"

#include <iostream>

namespace sflight {
namespace mdls {

Table3D::Table3D(const std::size_t numPages, double pageVals[])
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

std::size_t Table3D::getNumPages() { return numPages; }

void Table3D::multiply(const double val)
{
   for (std::size_t i = 0; i < numPages; i++) {
      data[i]->multiply(val);
   }
}

void Table3D::setPage(const std::size_t page, Table2D* table)
{
   if (page < numPages) {
      data[page] = table;
   }
}

Table2D* Table3D::getPage(const std::size_t page)
{
   if (page < numPages) {
      return data[page];
   }
   std::cout << "Page outside of bounds" << std::endl;
   std::exit(0);
}

double Table3D::interp(const double pageVal, const double rowVal, const double colVal)
{
   int lowpage{};
   int highpage{};
   double pageweight{};

   for (std::size_t i = 1; i < numPages; i++) {
      lowpage = i - 1;
      highpage = i;
      if (pageVals[i] > pageVal) {
         break;
      }
   }
   if (numPages > 1) {
      pageweight = (pageVal - pageVals[lowpage]) / (pageVals[highpage] - pageVals[lowpage]);
   }

   const double firstpage{data[lowpage]->interp(rowVal, colVal)};
   const double secpage{data[highpage]->interp(rowVal, colVal)};

   return firstpage + (secpage - firstpage) * pageweight;
}

void Table3D::print() const
{
   for (std::size_t i = 0; i < numPages; i++) {
      std::cout << "page: " << i << ", val: " << pageVals[i] << std::endl;
      data[i]->print();
   }
}
}
}
