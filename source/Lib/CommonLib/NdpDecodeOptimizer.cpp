#include "NdpDecodeOptimizer.h"

FILE *NdpDecoderOptimizer::baseMvLogFile, *NdpDecoderOptimizer::optReportFile;
std::map<std::string, MvLogData*> NdpDecoderOptimizer::mvLogDataMap;
std::map<std::string, std::list<MvLogData*> > NdpDecoderOptimizer::mvLogDataMapPerCTULine;
std::map<std::string, std::pair<std::pair<int, int>, double> > NdpDecoderOptimizer::prefFracMap;
std::map<std::string, std::pair<int, double> > NdpDecoderOptimizer::avgMvMap;
long long int NdpDecoderOptimizer::countAdjustedMVs, NdpDecoderOptimizer::totalDecodedMVs;
bool NdpDecoderOptimizer::isFracOnly;
int NdpDecoderOptimizer::frameWidth, NdpDecoderOptimizer::frameHeight;

double GLOBAL_TH = 0.0;

int NdpDecoderOptimizer::fracInterpDependencies[16][2][3] = {
    { {-1, -1, -1} , {-1, -1, -1} }, //0
    { { 2, -1, -1} , {-1, -1, -1} }, //1
    { {-1, -1, -1} , {-1, -1, -1} }, //2
    { { 2, -1, -1} , {-1, -1, -1} }, //3
    { { 8, -1, -1} , {-1, -1, -1} }, //4
    { { 8,  4,  6} , { 2,  1,  9} }, //5
    { { 2, 10, -1} , {-1, -1, -1} }, //6
    { { 8,  4,  6} , { 2,  3, 11} }, //7
    { {-1, -1, -1} , {-1, -1, -1} }, //8
    { { 8, 10, -1} , {-1, -1, -1} }, //9
    { { 8, -1, -1} , { 2, -1, -1} }, //10
    { { 8, 10, -1} , {-1, -1, -1} }, //11
    { { 8, -1, -1} , {-1, -1, -1} }, //12
    { { 8, 12, 14} , { 2,  1,  9} }, //13
    { { 2, 10, -1} , {-1, -1, -1} }, //14
    { { 8, 12, 14} , { 2,  3, 11} }   //15
};

int NdpDecoderOptimizer::INTERP_WINDOW_SIZE = 2048 * 128;


std::string NdpDecoderOptimizer::generateMvLogMapKey(int currFramePoc, PosType xPU, PosType yPU, int refList, int refFramePoc) {
    std::string key = std::to_string(currFramePoc) + "_" +
                          std::to_string(xPU) + "_" +
                          std::to_string(yPU) + "_" +
                          std::to_string(refList) + "_" +
                          std::to_string(refFramePoc);
    
    return key;
}

std::string NdpDecoderOptimizer::generateKeyPerCTULine(int currFramePoc, PosType xPU, PosType yPU, int refList) {

    int ctuLine = 0;
    if(frameWidth == 3840 || frameWidth == 4096) {
        ctuLine = (yPU / 128) * 2 + (xPU / (frameWidth / 2));
    }
    else {
        ctuLine = yPU / 128;
    }

    // for debug
    // std::cout << "(" << xPU << "," << yPU << ") --> " << ctuLine << std::endl;
    

    std::string key = std::to_string(currFramePoc) + "_" +
                          std::to_string(ctuLine) + "_" +
                          std::to_string(refList);
    
    return key;
}


void NdpDecoderOptimizer::openBaseMvLogFile(std::string fileName, int width, int height) {
    baseMvLogFile = fopen(fileName.c_str(), "r");

    frameWidth = width;
    frameHeight = height;

    optReportFile = fopen("decoder-opt.log", "w");

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

    while(!feof(baseMvLogFile)) {
        int res = fscanf(baseMvLogFile, "%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d\n", &currFramePoc, &xPU, &yPU, &wPU, &hPU, &refList, &refFramePoc, &xMV, &yMV, &xIntegMV, &yIntegMV, &xFracMV, &yFracMV);

        if(res == 0) {
            break;
        }
        MvLogData* mvData = new MvLogData();
        
        mvData->currFramePoc = currFramePoc;
        mvData->xPU = xPU;
        mvData->yPU = yPU;
        mvData->wPU = wPU;
        mvData->hPU = hPU;
        mvData->refList = refList;
        mvData->refFramePoc = refFramePoc;
        mvData->xMV = xMV;
        mvData->yMV = yMV;
        mvData->xIntegMV = xIntegMV;
        mvData->yIntegMV = yIntegMV;
        mvData->xFracMV = xFracMV;
        mvData->yFracMV = yFracMV;

        std::string key = generateMvLogMapKey(currFramePoc, xPU, yPU, refList, refFramePoc);
        mvLogDataMap.insert({key, mvData});

        std::string keyPerLine = generateKeyPerCTULine(currFramePoc, xPU, yPU, refList);
        if(mvLogDataMapPerCTULine.find(keyPerLine) != mvLogDataMapPerCTULine.end()) {
            mvLogDataMapPerCTULine.at(keyPerLine).push_back(mvData);
        }
        else {
            std::list<MvLogData*> list;
            list.push_back(mvData);
            mvLogDataMapPerCTULine.insert({keyPerLine, list});
        } 

    }

    fprintf(optReportFile, "ctu-line-id;cus-count;pref-frac;pref-frac-hit");
    if(!isFracOnly) {
        // std::cout << "AAAA " << isFracOnly << std::endl;
        fprintf(optReportFile, ";avg-mv;avg-mv-hit");
    }    
    fprintf(optReportFile, "\n");

    // for debug
    // std::cout << "Num of logged MVs: " << mvLogDataMap.size() << std::endl;
    // std::cout << "Num of logged CTU line keys: " << mvLogDataMapPerCTULine.size() << std::endl;

    for(auto it = mvLogDataMapPerCTULine.begin(); it != mvLogDataMapPerCTULine.end(); ++it) {
        std::string ctuLineKey = it->first;
        int cusWithinLine = it->second.size();

        std::pair<std::pair<int, int>, double> resultPrefFrac = calculatePrefFrac(it->second);
        prefFracMap.insert({it->first, resultPrefFrac});

        int prefFrac = resultPrefFrac.first.first;
        double prefFracHit = resultPrefFrac.second;

        fprintf(optReportFile, "%s;%d;%d;%.3f", ctuLineKey.c_str(), cusWithinLine, prefFrac, prefFracHit);

        // for debug
        // std::cout << "[" << ctuLineKey << "] --> CUs " << cusWithinLine << "\t| PrefFrac " << prefFrac << "\t| PfHit " << prefFracHit; 

        if(!isFracOnly) {
            std::pair<int, double> resultAvgMV = calculateAvgMV(it->second);
            avgMvMap.insert({it->first, resultAvgMV});

            int avgMv = resultAvgMV.first;
            double avgMvHit = resultAvgMV.second;

            fprintf(optReportFile, ";%d;%.3f", avgMv, avgMvHit);

            // for debug
            // std::cout << "\t| AvgMv " << avgMv << "\t| AvHit " << avgMvHit;
        }

        fprintf(optReportFile, "\n");

        // for debug
        // std::cout << std::endl;

        
    }

        
}

void NdpDecoderOptimizer::setOptMode(int cfgFracOnly) {
    
    isFracOnly = cfgFracOnly == 1;

    // std::cout << "Frac only mode: " << cfgFracOnly << std::endl;
}

MvLogData* NdpDecoderOptimizer::getMvData(int currFramePoc, PosType xPU, PosType yPU, int refList, int refFramePoc) {
    std::string key = generateMvLogMapKey(currFramePoc, xPU, yPU, refList, refFramePoc);
    if(mvLogDataMap.find(key) != mvLogDataMap.end()) {
        MvLogData* mvData = mvLogDataMap.at(key);
        return mvData;
    }
    else {
        return NULL;
    }
}

int NdpDecoderOptimizer::getFracPosition(int xFracMV, int yFracMV) {
    int xQuarterMV = xFracMV >> 2;
    int yQuarterMV = yFracMV >> 2;

    int fracPosition = (xQuarterMV << 2) | yQuarterMV;

    return fracPosition;
}


std::pair<std::pair<int, int>, double> NdpDecoderOptimizer::calculatePrefFrac(std::list<MvLogData*> list) {
    int countFracPos[16];

    for (int i = 0; i < 16; i++) {
        countFracPos[i] = 0;
    }   
   
    for(std::list<MvLogData*>::iterator it = list.begin(); it != list.end(); ++ it) {
        int fracPosition = getFracPosition((*it)->xFracMV, (*it)->yFracMV);

        // for debug
        // std::cout << "(" << (*it)->xFracMV << "," << (*it)->yFracMV << ") [" << fracPosition << "]\n";

        countFracPos[fracPosition] += (*it)->wPU * (*it)->hPU;
    }
 
    int countFracArea = 0;
    int prefFrac = -1;
    int maxOcc = -1;

    for (int frac = 1; frac < 16; frac++) {
        countFracArea += countFracPos[frac];
        if(countFracPos[frac] > maxOcc) {
            maxOcc = countFracPos[frac];
            prefFrac = frac;
        }
    }

    if(countFracArea != 0) {
        // double percentFrac = (maxOcc * 1.0) / countFracArea;
        int totalPrefFracOcc = maxOcc;

        int prefHalfFrac = countFracPos[2] >=  countFracPos[8] ? countFracPos[2] : countFracPos[8];

        for (int i = 0; i < 3; i++) {
            int fracDepList = 0;
            
            if(prefFrac == 5 || prefFrac == 7 || prefFrac == 9 || prefFrac == 11 ) {
                fracDepList = prefHalfFrac == 8 ? 0 : 1;
            }

            int fracDep = fracInterpDependencies[prefFrac][fracDepList][i];
            if(fracDep != -1) {
                
                totalPrefFracOcc += countFracPos[fracDep];
            }
        }

        double percentFrac = (totalPrefFracOcc * 1.0) / INTERP_WINDOW_SIZE;
        return std::pair<std::pair<int, int>, double>(std::pair<int, int>(prefFrac, prefHalfFrac), percentFrac);
    }
    else {
        return std::pair<std::pair<int, int>, double>(std::pair<int, int>(-1, -1), -1);
    }
}


std::pair<int, double> NdpDecoderOptimizer::calculateAvgMV(std::list<MvLogData*> list) {
    int yCount = 0;
    int accumFracPUs = 0;
    
    for(std::list<MvLogData*>::iterator it = list.begin(); it != list.end(); ++ it) {
        int fracPosition = getFracPosition((*it)->xFracMV, (*it)->yFracMV);
        bool isFrac = fracPosition != 0;
        if(isFrac) {
            accumFracPUs ++;
            yCount += (*it)->yIntegMV;
        }
    }

    
    bool isNeg = yCount < 0;

    double yAvgDouble = abs(yCount) * 1.0 / accumFracPUs;
    int yAvg = (isNeg ? -round(yAvgDouble) : round(yAvgDouble));
    
    //yAvg = 0; //for debug!

    int yTop = yAvg;
    int yBottom = yAvg + 128;

    int accumMVsInsideInterpWindow = 0;
    for(std::list<MvLogData*>::iterator it = list.begin(); it != list.end(); ++ it) {
        int yIntegMV = (*it)->yIntegMV;
        SizeType hPU = (*it)->hPU;

        int fracPosition = getFracPosition((*it)->xFracMV, (*it)->yFracMV);
        bool isFrac = fracPosition != 0;

        if(isFrac) {
            if(yIntegMV >= yTop && (yIntegMV + hPU) < yBottom) {
                accumMVsInsideInterpWindow ++;
            }
        }
    }

    if(accumFracPUs == 0) {
        return std::pair<int, double>(-6666, -1);
    }
    else {
        double percentInsideInterpWindow = (accumMVsInsideInterpWindow * 1.0) / accumFracPUs;
        return std::pair<int, double>(yAvg, percentInsideInterpWindow);
    }

}

int NdpDecoderOptimizer::fracPosToBeAdjusted(double prefFracHit, int fracPos, int prefFrac, int prefHalfFrac) {
    // for debug
    // std::cout << "FRAC: (" << prefFracHit << ") " << fracPos << " --> " << prefFrac << " ==> ";

    if(prefFracHit < GLOBAL_TH) {  // Threshold to disable MVs adjustments according to prefFracHit
        // std::cout << "ThSkip" << std::endl;
        return fracPos; // no adjusts
    }

    if(fracPos == prefFrac) {
        // std::cout << "PrefFracHit" << std::endl;
        return fracPos; // no adjusts
    }   

    int fracDepList = 0;
        
    if(prefFrac == 5 || prefFrac == 7 || prefFrac == 9 || prefFrac == 11 ) {
        fracDepList = prefHalfFrac == 8 ? 0 : 1;
    }

    for (int i = 0; i < 3; i++) {
        if(fracPos == fracInterpDependencies[prefFrac][fracDepList][i]) {
            // std::cout << "PrefFracDependHit" << std::endl;
            return fracPos; // no adjusts
        }
    }      

    // std::cout << "PrefFracMiss" << std::endl;
    return prefFrac; // adjusts frac MV pos to prefFrac
    
}

void NdpDecoderOptimizer::modifyMV(int currFramePoc, PosType xPU, PosType yPU, SizeType hPU, int refList, int refFramePoc, int* xMV, int* yMV) {
    int yIntegMV = (*yMV) >> MV_FRAC_BITS_LUMA;
    int xFracMV = (*xMV) & MV_FRAC_MASK_LUMA, yFracMV = (*yMV) & MV_FRAC_MASK_LUMA; 

    int fracPosition = getFracPosition(xFracMV, yFracMV);
       
    std::string ctuLineKey = generateKeyPerCTULine(currFramePoc, xPU, yPU, refList);

    if(prefFracMap.find(ctuLineKey) == prefFracMap.end()) {
        return;
    }

    if(! isFracOnly) {
        if(avgMvMap.find(ctuLineKey) == avgMvMap.end()) {
            return;
        }
    }
    
    std::pair<std::pair<int, int>, double> prefFracResult = prefFracMap.at(ctuLineKey);

    int prefFrac = prefFracResult.first.first;
    int prefHalfFrac = prefFracResult.first.second;
    double prefFracHit = prefFracResult.second;

    std::pair<int, double> avgMVResult;

    bool isFrac = fracPosition != 0;

    int xMVBkp = *xMV & (~ 0x3); //zeroing 2 LSB (signaling) of MV bkp
    int yMVBkp = *yMV & (~ 0x3); //zeroing 2 LSB (signaling) of MV bkp

    totalDecodedMVs ++;
    
    if(prefFrac == -1 || !isFrac)
        return;

    if(! isFracOnly) {
        avgMVResult = avgMvMap.at(ctuLineKey);
    
        if(avgMVResult.first == -6666) 
            return;

        int yTop = avgMVResult.first;
        int yBottom = avgMVResult.first + 128;

        bool adjustMV = false;    
        int yAdjustedMV = 0;

        if(yIntegMV < yTop) {
            adjustMV = true;
            yAdjustedMV = yTop;
        }
        else {
            if((yIntegMV + hPU) > yBottom) {
                adjustMV = true;
                yAdjustedMV = yBottom - hPU;
            }
        }

        // Adjusting integer position to the avg mv of the current CTU Line
        if(adjustMV) {
            int yIntegMask = (yAdjustedMV << 4) | yFracMV;

            (*yMV) = yIntegMask;
        }
    }

    int fracPosAdjusted = fracPosToBeAdjusted(prefFracHit, fracPosition, prefFrac, prefHalfFrac);


    // Adjusting frac position to the prefFrac of the current CTU Line
    int xFracMask = (fracPosAdjusted >> 2) << 2;
    int yFracMask = (fracPosAdjusted & 0x3) << 2;

    (*xMV) = ((*xMV) & 0xFFFFFFF0) | xFracMask;  // Zeroing last two bits (signaling)
    (*yMV) = ((*yMV) & 0xFFFFFFF0) | yFracMask;  // Zeroing last two bits (signaling)

    if((*xMV) != xMVBkp || (*yMV) != yMVBkp) {
        countAdjustedMVs ++;
        
        // for debug
        // std::cout << "(" << xMVBkp << "," << yMVBkp << ") -> \t (" << (*xMV) << "," << (*yMV) << ") [" << fracPosition << "x" << prefFracResult.first << "] {" << yIntegMV << "x" << avgMVResult.first << "}\n";   
    }

}

void NdpDecoderOptimizer::logDecoderOptSummary() {
    double adjustedMvsPercent = (countAdjustedMVs * 1.0) / totalDecodedMVs;
        
    fprintf(optReportFile, "adj-mvs-pctg;%.2f\n", adjustedMvsPercent);

    fclose(optReportFile);

}