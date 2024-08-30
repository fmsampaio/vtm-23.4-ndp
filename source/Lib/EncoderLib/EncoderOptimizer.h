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
        static std::map<std::string, std::pair<int, double> > prefFracMap;

        static RefPicList currRefList;
        static PosType currYCu;  
        static int currFramePocStatic;

        static void xStoreCuMotionData(int currFramePoc, PosType xCU, PosType yCU, SizeType wCU, SizeType hCU, int refList, int refFramePoc, Mv origMv, Mv shiftMv);
        static int xGetFracPosition(Mv shiftMv);
        static std::pair<int, double> xCalculatePrefFrac(std::string ctuLineKey);         

    public:
        static void traceCtuCodingInfo(const CodingStructure& cs, const UnitArea& ctuArea);
        static void storeCtuMotionData(const CodingStructure& cs, const UnitArea& ctuArea);

        static std::pair<int, double> getPrefFrac();
        static void updatePrefFracMap();

        static int getFracPosition(Mv mv, MvPrecision precision);

        static void reportMotionData(UnitArea ctuArea);

        // helper functions
        static bool isFirstCtuInLine(UnitArea ctuArea);
        static bool isHalfPel(int fracPos);
        static bool isQuarterPel(int fracPos);

        static int getHalfPosition(int fracPos);
        static int getQuarterPosition(int fracPos);

        static void setCurrRefList(RefPicList refList) { currRefList = refList; }
        static RefPicList getCurrRefList() { return currRefList; }

        static void setCurrYCu(PosType yCU) { currYCu = yCU; }
        static PosType getCurrYCu() { return currYCu; }

        static void setCurrFramePocStatic(int currFramePoc) { currFramePocStatic = currFramePoc; }
        static int getCurrFramePocStatic() { return currFramePocStatic; }

        static Mv shiftMvFromInternalToQuarter(Mv &mv);

};