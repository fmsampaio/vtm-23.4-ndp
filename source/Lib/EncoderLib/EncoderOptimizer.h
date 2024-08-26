#include "CommonLib/CodingStructure.h"
#include "CommonLib/UnitTools.h"
#include "CommonLib/dtrace_next.h"

class MotionData {
    public:
        int currFramePoc;
        PosType xCU;
        PosType yCU;
        SizeType wCU;
        SizeType hCU;
        int refList;
        int refFramePoc;
        Mv origMv;
        Mv shiftMv;
        int fracPos;
};

class EncoderOptimizer {
    private: 
        static std::map<std::string, std::list<MotionData*> > motionDataMap;

        static void xStoreCuMotionData(int currFramePoc, PosType xCU, PosType yCU, SizeType wCU, SizeType hCU, int refList, int refFramePoc, Mv origMv, Mv shiftMv);

    public:
        static void traceCtuCodingInfo(const CodingStructure& cs, const UnitArea& ctuArea);
        static void storeCtuMotionData(const CodingStructure& cs, const UnitArea& ctuArea);
        static void reportMotionData();

};