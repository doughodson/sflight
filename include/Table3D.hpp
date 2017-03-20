
#ifndef TABLE3D_H
#define TABLE3D_H

#include "Table2D.hpp"


namespace SimpleFlight {
    
    
class Table3D {    
    
    public:

    Table3D();
    Table3D( const int numPages, double pageVals[]);
    ~Table3D();
    
    
    int getNumPages();
    
    void multiply(double val);
    
    void setPage(int page, Table2D *table);
    
    Table2D* getPage(int page);
    
    double interp(double pageVal, double rowVal, double colVal);
    
    void print();
    
    private:
    
    Table2D **data;
    double* pageVals;
    int numPages;

};

};

#endif
