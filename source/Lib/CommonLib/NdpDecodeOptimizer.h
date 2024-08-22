#include <cstdio>
#include <string>
#include <map>
#include <list>
#include <iostream>
#include <cmath>

using PosType  = int32_t;
using SizeType = uint32_t;

class MvLogData {
    public:
        int currFramePoc;
        PosType xPU;
        PosType yPU;
        SizeType wPU;
        SizeType hPU;
        int refList;
        int refFramePoc;
        int xMV;
        int yMV;
        int xIntegMV;
        int yIntegMV;
        int xFracMV;
        int yFracMV;
};

class NdpDecoderOptimizer {
    private:
        static FILE *baseMvLogFile, *optMvLogFile;
        static std::map<std::string, MvLogData*> mvLogDataMap;
        static std::map<std::string, std::list<MvLogData*> > mvLogDataMapPerCTULine;
        static std::map<std::string, std::pair<int, double> > prefFracMap;
        static std::map<std::string, std::pair<int, double> > avgMvMap;

    public: 
        static void openBaseMvLogFile(std::string fileName);
        static std::string generateMvLogMapKey(int currFramePoc, PosType xPU, PosType yPU, int refList, int refFramePoc);
        static std::string generateKeyPerCTULine(int currFramePoc, PosType yPU, int refList);
        static MvLogData* getMvData(int currFramePoc, PosType xPU, PosType yPU, int refList, int refFramePoc);   
        static std::pair<int, double> calculatePrefFrac(std::list<MvLogData*> list);
        static int getFracPosition(int xFracMV, int yFracMV);
        
};