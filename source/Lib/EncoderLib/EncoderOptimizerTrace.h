#include "CommonLib/CodingStructure.h"
#include "CommonLib/UnitTools.h"
#include "CommonLib/dtrace_next.h"

class EncoderOptimizerTrace {
    private: 

    public:
        static void traceCtuCodingInfo(const CodingStructure& cs, const UnitArea& ctuArea, Position ctuPos);
};