
#ifndef __sflight_mdls_Table3D_H__
#define __sflight_mdls_Table3D_H__

#include "sflight/mdls/Table2D.hpp"

#include <cstddef>

namespace sflight {
namespace mdls {

//------------------------------------------------------------------------------
// Class: Table3D
//------------------------------------------------------------------------------
class Table3D
{
public:
   Table3D() = delete;
   Table3D(const std::size_t numPages, double pageVals[]);
   virtual ~Table3D();

   std::size_t getNumPages();
   void multiply(const double val);
   void setPage(const std::size_t page, Table2D* table);
   Table2D* getPage(const std::size_t);
   double interp(const double pageVal, const double rowVal, const double colVal);
   void print() const;

private:
   Table2D** data{};
   double* pageVals{};
   std::size_t numPages{};
};

}
}

#endif
