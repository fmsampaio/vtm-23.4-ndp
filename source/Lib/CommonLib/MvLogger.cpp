#include "MvLogger.h"

FILE* MvLogger::loggerFile;
bool MvLogger::decoding = false;
RefPicList MvLogger::currRefList;

void MvLogger::init() {
    loggerFile = fopen("vectors-opt.log", "w"); 
    setDecoding(true);
}

void MvLogger::close() {  
    fclose(loggerFile);
}

void MvLogger::setDecoding(bool isDecoding) {
    decoding = isDecoding;
}

bool MvLogger::isDecoding() {
    return decoding;
}

std::pair<int, int> MvLogger::extractIntegAndFrac(int *xCoord, int *yCoord) {
    int xFrac = *xCoord & MV_FRAC_MASK_LUMA;
    int yFrac = *yCoord & MV_FRAC_MASK_LUMA;

    *xCoord = *xCoord >> MV_FRAC_BITS_LUMA;
    *yCoord = *yCoord >> MV_FRAC_BITS_LUMA;

    return std::pair<int, int>(xFrac, yFrac);
}

void MvLogger::logMotionVector(
        int currFramePoc,
        PosType xPU, 
        PosType yPU,
        SizeType wPU,
        SizeType hPU,
        int refList,
        int refFramePoc,
        int xMV,
        int yMV
    ) {

        int xIntegMV = xMV;
        int yIntegMV = yMV;

        std::pair<int, int> fracMV = extractIntegAndFrac(&xIntegMV, &yIntegMV);

        int xFracMV = fracMV.first;
        int yFracMV = fracMV.second;
        
        fprintf(loggerFile, "%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d\n",
        currFramePoc, xPU, yPU, wPU, hPU, refList, refFramePoc, xMV, yMV, xIntegMV, yIntegMV, xFracMV, yFracMV);
}