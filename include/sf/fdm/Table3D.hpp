
#ifndef __sf_fdm_Table3D_H__
#define __sf_fdm_Table3D_H__

#include "sf/fdm/Table2D.hpp"

namespace sf {
namespace fdm {

//------------------------------------------------------------------------------
// Class: Table3D
//------------------------------------------------------------------------------
class Table3D {
 public:
   Table3D() = delete;
   Table3D(const int numPages, double pageVals[]);
   virtual ~Table3D();

   int getNumPages();
   void multiply(const double val);
   void setPage(const int page, Table2D* table);
   Table2D* getPage(const int page);
   double interp(const double pageVal, const double rowVal,
                 const double colVal);
   void print() const;

 private:
   Table2D** data{};
   double* pageVals{};
   int numPages{};
};
}
}

#endif
