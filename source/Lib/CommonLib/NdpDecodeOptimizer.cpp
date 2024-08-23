#include "NdpDecodeOptimizer.h"

FILE *NdpDecoderOptimizer::baseMvLogFile, *NdpDecoderOptimizer::optReportFile;
std::map<std::string, MvLogData*> NdpDecoderOptimizer::mvLogDataMap;
std::map<std::string, std::list<MvLogData*> > NdpDecoderOptimizer::mvLogDataMapPerCTULine;
std::map<std::string, std::pair<int, double> > NdpDecoderOptimizer::prefFracMap;
std::map<std::string, std::pair<int, double> > NdpDecoderOptimizer::avgMvMap;
long long int NdpDecoderOptimizer::countAdjustedMVs, NdpDecoderOptimizer::totalDecodedMVs;
bool NdpDecoderOptimizer::isFracOnly;

std::string NdpDecoderOptimizer::generateMvLogMapKey(int currFramePoc, PosType xPU, PosType yPU, int refList, int refFramePoc) {
    std::string key = std::to_string(currFramePoc) + "_" +
                          std::to_string(xPU) + "_" +
                          std::to_string(yPU) + "_" +
                          std::to_string(refList) + "_" +
                          std::to_string(refFramePoc);
    
    return key;
}

std::string NdpDecoderOptimizer::generateKeyPerCTULine(int currFramePoc, PosType yPU, int refList) {
    int ctuLine = yPU / 128;

    std::string key = std::to_string(currFramePoc) + "_" +
                          std::to_string(ctuLine) + "_" +
                          std::to_string(refList);
    
    return key;
}


void NdpDecoderOptimizer::openBaseMvLogFile(std::string fileName) {
    baseMvLogFile = fopen(fileName.c_str(), "r");

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

        std::string keyPerLine = generateKeyPerCTULine(currFramePoc, yPU, refList);
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
        std::cout << "AAAA " << isFracOnly << std::endl;
        fprintf(optReportFile, ";avg-mv;avg-mv-hit");
    }    
    fprintf(optReportFile, "\n");

    // for debug
    // std::cout << "Num of logged MVs: " << mvLogDataMap.size() << std::endl;
    // std::cout << "Num of logged CTU line keys: " << mvLogDataMapPerCTULine.size() << std::endl;

    for(auto it = mvLogDataMapPerCTULine.begin(); it != mvLogDataMapPerCTULine.end(); ++it) {
        std::string ctuLineKey = it->first;
        int cusWithinLine = it->second.size();

        std::pair<int, double> resultPrefFrac = calculatePrefFrac(it->second);
        prefFracMap.insert({it->first, resultPrefFrac});

        int prefFrac = resultPrefFrac.first;
        double prefFracHit = resultPrefFrac.second;

        fprintf(optReportFile, "%s;%d;%d;%.3f", ctuLineKey.c_str(), cusWithinLine, prefFrac, prefFracHit);

        if(!isFracOnly) {
            std::pair<int, double> resultAvgMV = calculateAvgMV(it->second);
            avgMvMap.insert({it->first, resultAvgMV});

            int avgMv = resultAvgMV.first;
            double avgMvHit = resultAvgMV.second;

            fprintf(optReportFile, ";%d;%.3f", avgMv, avgMvHit);
        }

        fprintf(optReportFile, "\n");

        // for debug
        // std::cout << "[" << ctuLineKey << "] --> CUs " << cusWithinLine << "\t| PrefFrac " << prefFrac << "\t| PfHit " << prefFracHit << "\t| AvgMv " << avgMv << "\t| AvHit " << avgMvHit << std::endl;
    }

        
}

void NdpDecoderOptimizer::setOptMode(int cfgFracOnly) {
    
    isFracOnly = cfgFracOnly == 1;

    std::cout << "Frac only mode: " << cfgFracOnly << std::endl;
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


std::pair<int, double> NdpDecoderOptimizer::calculatePrefFrac(std::list<MvLogData*> list) {
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
        double percentFrac = (maxOcc * 1.0) / countFracArea;
        return std::pair<int, double>(prefFrac, percentFrac);
    }
    else {
        return std::pair<int, double>(-1, -1);
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

void NdpDecoderOptimizer::modifyMV(int currFramePoc, PosType xPU, PosType yPU, SizeType hPU, int refList, int refFramePoc, int* xMV, int* yMV) {
    int yIntegMV = (*yMV) >> MV_FRAC_BITS_LUMA;
    int xFracMV = (*xMV) & MV_FRAC_MASK_LUMA, yFracMV = (*yMV) & MV_FRAC_MASK_LUMA;    

    int fracPosition = getFracPosition(xFracMV, yFracMV);
       
    std::string ctuLineKey = generateKeyPerCTULine(currFramePoc, yPU, refList);

    if(prefFracMap.find(ctuLineKey) == prefFracMap.end()) {
        return;
    }

    if(! isFracOnly) {
        if(avgMvMap.find(ctuLineKey) == avgMvMap.end()) {
            return;
        }
    }
    
    std::pair<int, double> prefFracResult = prefFracMap.at(ctuLineKey);

    std::pair<int, double> avgMVResult;

    bool isFrac = fracPosition != 0;

    int xMVBkp = *xMV;
    int yMVBkp = *yMV;

    totalDecodedMVs ++;
    
    if(prefFracResult.first == -1 || !isFrac)
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

    // Adjusting frac position to the prefFrac of the current CTU Line
    int xFracMask = (prefFracResult.first >> 2) << 2;
    int yFracMask = (prefFracResult.first & 0x3) << 2;

    (*xMV) = ((*xMV) & 0xFFFFFFF3) | xFracMask;
    (*yMV) = ((*yMV) & 0xFFFFFFF3) | yFracMask;

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