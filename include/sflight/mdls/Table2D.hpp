
#ifndef __sflight_mdls_Table2D_H__
#define __sflight_mdls_Table2D_H__

#include <string>
#include <cstddef>

namespace sflight {
namespace mdls {

//------------------------------------------------------------------------------
// Class: Table2D
//------------------------------------------------------------------------------
class Table2D
{
public:
   Table2D() = delete;
   Table2D(const std::size_t numRows, const std::size_t numCols, double rowVals[], double colVals[]);
   virtual ~Table2D();

   std::size_t getNumCols();
   std::size_t getNumRows();
   void multiply(const double val);
   void setRowData(const std::size_t row, double rowdata[]);
   void setData(std::string& tabledata);

   double** getData();
   double* getColVals();
   double* getRowVals();

   void set(const std::size_t row, const std::size_t col, const double val);
   double get(const std::size_t row, const std::size_t col);
   double interp(const double rowVal, const double colVal);
   void print();

 private:
   double** data{};

   std::size_t numRows{};
   std::size_t numCols{};

   double* rowVals{};
   double* colVals{};
};

}
}

#endif
