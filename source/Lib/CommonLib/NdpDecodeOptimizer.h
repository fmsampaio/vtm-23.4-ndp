#include <cstdio>
#include <string>
#include <map>
#include <list>
#include <iostream>
#include <cmath>

#include "CommonDef.h"

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
        static int fracInterpDependencies[16][2][3];
        static int INTERP_WINDOW_SIZE;

        static FILE *baseMvLogFile, *optReportFile;
        static std::map<std::string, MvLogData*> mvLogDataMap;
        static std::map<std::string, std::list<MvLogData*> > mvLogDataMapPerCTULine;
        static std::map<std::string, std::pair<std::pair<int, int>, double> > prefFracMap;
        static std::map<std::string, std::pair<int, double> > avgMvMap;

        static long long int countAdjustedMVs;
        static long long int totalDecodedMVs;
        
        static bool isFracOnly;

        static int frameWidth, frameHeight;
        static double optClscTh;

    public: 
        static void openBaseMvLogFile(std::string fileName, int width, int height);
        static void setOptMode(int fracOnlyCfg);
        static void setOptClscTh(double th);
        
        static std::string generateMvLogMapKey(int currFramePoc, PosType xPU, PosType yPU, int refList, int refFramePoc);
        static std::string generateKeyPerCTULine(int currFramePoc, PosType xPU, PosType yPU, int refList);
        static MvLogData* getMvData(int currFramePoc, PosType xPU, PosType yPU, int refList, int refFramePoc);   
        static std::pair<std::pair<int, int>, double> calculatePrefFrac(std::list<MvLogData*> list);
        static int getFracPosition(int xFracMV, int yFracMV);
        static std::pair<int, double> calculateAvgMV(std::list<MvLogData*> list);
        static void modifyMV(int currFramePoc, PosType xPU, PosType yPU, SizeType hPU, int refList, int refFramePoc, int* xMV, int* yMV);
        static void logDecoderOptSummary();

        static int fracPosToBeAdjusted(double prefFracHit, int fracPos, int prefFrac, int prefHalfFrac);
        
        
};