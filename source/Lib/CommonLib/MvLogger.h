#include <cstdio>

#include "CommonDef.h"

using PosType  = int32_t;
using SizeType = uint32_t;

class MvLogger {
    private:
        static FILE* loggerFile;
        static bool decoding;
        static RefPicList currRefList;

    public:
        static void init();
        static void close();

        static void setDecoding(bool isDecoding);
        static bool isDecoding();

        static void setRefFrameList(RefPicList refList);

        static void logMotionVector(
            int currFramePoc,
            PosType xPU, 
            PosType yPU,
            SizeType wPU,
            SizeType hPU,
            int refList,
            int refFramePoc,
            int xMV,
            int yMV
        );

        static std::pair<int, int> extractIntegAndFrac(int *xCoord, int *yCoord);
};