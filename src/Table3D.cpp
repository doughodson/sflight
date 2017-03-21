
#include "Table3D.hpp"

#include <iostream>

namespace sf
{

Table3D::Table3D(const int numPages, double pageVals[])
    : numPages(numPages), pageVals(pageVals)
{
   data = new Table2D*[numPages];
}

Table3D::~Table3D()
{
   if (data)
   {
      delete[] data;
   }
   delete[] pageVals;
}

int Table3D::getNumPages()
{
   return this->numPages;
}

void Table3D::multiply(double val)
{
   for (int i = 0; i < numPages; i++)
   {
      data[i]->multiply(val);
   }
}

void Table3D::setPage(int page, Table2D *table)
{
   if (page < numPages)
   {
      data[page] = table;
   }
}

Table2D *Table3D::getPage(int page)
{
   if (page < numPages)
   {
      return data[page];
   }
}

double Table3D::interp(double pageVal, double rowVal, double colVal)
{

   int lowpage = 0;
   int highpage = 0;
   double pageweight = 0;

   for (int i = 1; i < numPages; i++)
   {
      lowpage = i - 1;
      highpage = i;
      if (pageVals[i] > pageVal)
      {
         break;
      }
   }
   if (numPages > 1)
      pageweight = (pageVal - pageVals[lowpage]) / (pageVals[highpage] - pageVals[lowpage]);

   double firstpage = data[lowpage]->interp(rowVal, colVal);
   double secpage = data[highpage]->interp(rowVal, colVal);

   return firstpage + (secpage - firstpage) * pageweight;
}

void Table3D::print()
{
   for (int i = 0; i < numPages; i++)
   {

      std::cout << "page: " << i << ", val: " << pageVals[i] << std::endl;
      data[i]->print();
   }
}
}
