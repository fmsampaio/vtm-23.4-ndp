#include "NdpDecodeOptimizer.h"

FILE *NdpDecoderOptimizer::baseMvLogFile, *NdpDecoderOptimizer::optMvLogFile;
std::map<std::string, MvLogData*> NdpDecoderOptimizer::mvLogDataMap;
std::map<std::string, std::list<MvLogData*> > NdpDecoderOptimizer::mvLogDataMapPerCTULine;
std::map<std::string, std::pair<int, double> > NdpDecoderOptimizer::prefFracMap;
std::map<std::string, std::pair<int, double> > NdpDecoderOptimizer::avgMvMap;

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

    // for debug
    std::cout << "Num of logged MVs: " << mvLogDataMap.size() << std::endl;
    std::cout << "Num of logged CTU line keys: " << mvLogDataMapPerCTULine.size() << std::endl;

    for(auto it = mvLogDataMapPerCTULine.begin(); it != mvLogDataMapPerCTULine.end(); ++it) {
        std::string ctuLineKey = it->first;
        int cusWithinLine = it->second.size();

        std::cout << "[" << ctuLineKey << "] --> " << cusWithinLine << std::endl;
    }
        
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


