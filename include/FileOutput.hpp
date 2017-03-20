
#ifndef FileOutput_H
#define FileOutput_H

#include <fstream>
#include "FDMModule.hpp"

namespace sf
{

class FDMGlobals;
class Node;

class FileOutput : public virtual FDMModule
{
public:

      // functions from FDMModule
        void initialize(Node* node);
        void update( double timestep );

      FileOutput(FDMGlobals *globals, double frameRate);
        //FileOutput(FDMGlobals *globals, char* filename, int skipFrames);
        ~FileOutput();

        void update();
        void close();

        //FDMGlobals *globals;
        std::ofstream fout;
        int rate;
      double lastTime;
        int frameCounter;

};

}

#endif
