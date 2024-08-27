#include "EncoderOptimizer.h"

std::map<std::string, std::list<MotionData*> > EncoderOptimizer::motionDataMap;

void EncoderOptimizer::traceCtuCodingInfo(const CodingStructure& cs, const UnitArea& ctuArea) {
    
    DTRACE( g_trace_ctx, D_NDP_TRACE, "POC. %3d CTU: @(%4d,%4d) [%2dx%2d]\n",
        cs.slice->getPOC(),
        ctuArea.Y().x, ctuArea.Y().y,
        ctuArea.Y().width, ctuArea.Y().height
    );
    
    const ChannelType chType = ChannelType( ChannelType::LUMA );
    for( const CodingUnit &cu : cs.traverseCUs( CS::getArea( cs, ctuArea, chType ), chType ) ) {
        DTRACE( g_trace_ctx, D_NDP_TRACE, "CU: (%4d,%4d) [%2dx%2d] %s\n",
            cu.lx(),  
            cu.ly(),
            cu.lwidth(),
            cu.lheight(),
            cu.predMode == PredMode::MODE_INTER ? "Inter" : "Intra"
        );

        if (cu.predMode == MODE_INTER) { 
            const int nShift = MV_FRACTIONAL_BITS_DIFF;
            const int nOffset = 1 << (nShift - 1);

            for (const PredictionUnit &pu : CU::traversePUs(cu)) {
                if(pu.cu->affine) {
                    DTRACE( g_trace_ctx, D_NDP_TRACE, "Affine!\n");
                }
                else if(pu.cu->geoFlag) {
                    DTRACE( g_trace_ctx, D_NDP_TRACE, "Geo!\n");
                }
                else {
                    if(pu.interDir != 2) { // PRED_L0
                        int8_t refFramePoc = pu.cu->slice->getRefPic( REF_PIC_LIST_0, pu.refIdx[REF_PIC_LIST_0] )->getPOC();
                        Mv mvOrig = pu.mv[REF_PIC_LIST_0];
                        Mv mv;
                        mv.hor = mvOrig.hor >= 0 ? (mvOrig.hor + nOffset) >> nShift : -((-mvOrig.hor + nOffset) >> nShift);
                        mv.ver = mvOrig.ver >= 0 ? (mvOrig.ver + nOffset) >> nShift : -((-mvOrig.ver + nOffset) >> nShift);
                        DTRACE( g_trace_ctx, D_NDP_TRACE, "L0: Ref. %2d (%3d, %3d) (%3d, %3d)\n",
                            refFramePoc,
                            mvOrig.getHor(),
                            mvOrig.getVer(),
                            mv.getHor(),
                            mv.getVer()
                        );
                    }
                    if (pu.interDir != 1) {
                        int8_t refFramePoc = pu.cu->slice->getRefPic( REF_PIC_LIST_1, pu.refIdx[REF_PIC_LIST_1] )->getPOC();
                        Mv mvOrig = pu.mv[REF_PIC_LIST_1];
                        Mv mv;
                        mv.hor = mvOrig.hor >= 0 ? (mvOrig.hor + nOffset) >> nShift : -((-mvOrig.hor + nOffset) >> nShift);
                        mv.ver = mvOrig.ver >= 0 ? (mvOrig.ver + nOffset) >> nShift : -((-mvOrig.ver + nOffset) >> nShift);
                        DTRACE( g_trace_ctx, D_NDP_TRACE, "L1: Ref. %2d (%3d, %3d) (%3d, %3d)\n",
                            refFramePoc,
                            mvOrig.getHor(),
                            mvOrig.getVer(),
                            mv.getHor(),
                            mv.getVer()
                        );
                    }
                }
            }
        }      
    }
}

static std::string xGenerateCtuLineKey(int currFramePoc, PosType yCU, int refList) {
    int ctuLine = yCU / MAX_CU_SIZE;

    std::string key = std::to_string(currFramePoc) + "_" +
                          std::to_string(ctuLine) + "_" +
                          std::to_string(refList);
    
    return key;
}

void EncoderOptimizer::storeCtuMotionData(const CodingStructure& cs, const UnitArea& ctuArea) {
    int currFramePoc = cs.slice->getPOC();

    const ChannelType chType = ChannelType( ChannelType::LUMA );
    for( const CodingUnit &cu : cs.traverseCUs( CS::getArea( cs, ctuArea, chType ), chType ) ) {
        int xCU = cu.lx();
        int yCU = cu.ly();
        int wCU = cu.lwidth();
        int hCU = cu.lheight();

        if (cu.predMode == MODE_INTER) { 
            const int nShift = MV_FRACTIONAL_BITS_DIFF;
            const int nOffset = 1 << (nShift - 1);

            for (const PredictionUnit &pu : CU::traversePUs(cu)) {
                if(!pu.cu->affine && !pu.cu->geoFlag) {
                    if(pu.interDir != 2) { // PRED_L0
                        int8_t refFramePoc = pu.cu->slice->getRefPic( REF_PIC_LIST_0, pu.refIdx[REF_PIC_LIST_0] )->getPOC();
                        int refList = REF_PIC_LIST_0;
                        Mv origMv, shiftMv;
                        origMv.hor = pu.mv[REF_PIC_LIST_0].hor;
                        origMv.ver = pu.mv[REF_PIC_LIST_0].ver;
                        shiftMv.hor = origMv.hor >= 0 ? (origMv.hor + nOffset) >> nShift : -((-origMv.hor + nOffset) >> nShift);
                        shiftMv.ver = origMv.ver >= 0 ? (origMv.ver + nOffset) >> nShift : -((-origMv.ver + nOffset) >> nShift);
                        xStoreCuMotionData(currFramePoc, xCU, yCU, wCU, hCU, refList, refFramePoc, origMv, shiftMv);
                    }
                    if (pu.interDir != 1) {
                        int8_t refFramePoc = pu.cu->slice->getRefPic( REF_PIC_LIST_1, pu.refIdx[REF_PIC_LIST_1] )->getPOC();
                        int refList = REF_PIC_LIST_1;
                        Mv origMv, shiftMv;
                        origMv.hor = pu.mv[REF_PIC_LIST_1].hor;
                        origMv.ver = pu.mv[REF_PIC_LIST_1].ver;
                        shiftMv.hor = origMv.hor >= 0 ? (origMv.hor + nOffset) >> nShift : -((-origMv.hor + nOffset) >> nShift);
                        shiftMv.ver = origMv.ver >= 0 ? (origMv.ver + nOffset) >> nShift : -((-origMv.ver + nOffset) >> nShift);
                        xStoreCuMotionData(currFramePoc, xCU, yCU, wCU, hCU, refList, refFramePoc, origMv, shiftMv);
                    }
                }
            }
        }
    }
}



std::pair<int, double> EncoderOptimizer::calculatePrefFrac(int currFramePoc, PosType yCU, int refList) {
    std::string ctuLineKey = xGenerateCtuLineKey(currFramePoc, yCU, refList);

    return calculatePrefFrac(ctuLineKey);
}

std::pair<int, double> EncoderOptimizer::calculatePrefFrac(std::string ctuLineKey) {

    if(motionDataMap.find(ctuLineKey) == motionDataMap.end()) {
        return std::pair<int, double>(-1, -1);
    }

    int countFracPos[16];
    for (int i = 0; i < 16; i++) {
        countFracPos[i] = 0;
    }  

    std::list<MotionData*> motionDataList = motionDataMap.at(ctuLineKey);
    for(std::list<MotionData*>::iterator it = motionDataList.begin(); it != motionDataList.end(); ++ it) {
        int fracPos = (*it)->fracPos;
        countFracPos[fracPos] += (*it)->wCU * (*it)->hCU;
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

int EncoderOptimizer::xGetFracPosition(Mv shiftMv) {
    int xQuarterMV = shiftMv.hor & 0x3;
    int yQuarterMV = shiftMv.ver & 0x3;

    int fracPosition = (xQuarterMV << 2) | yQuarterMV;

    return fracPosition;
}

void EncoderOptimizer::xStoreCuMotionData(int currFramePoc, PosType xCU, PosType yCU, SizeType wCU, SizeType hCU, int refList, int refFramePoc, Mv origMv, Mv shiftMv) {
    MotionData *md = new MotionData();
    md->currFramePoc = currFramePoc;
    md->xCU = xCU;
    md->yCU = yCU;
    md->wCU = wCU;
    md->hCU = hCU;
    md->refList = refList;
    md->refFramePoc = refFramePoc;
    md->origMv = origMv;
    md->shiftMv = shiftMv;
    md->fracPos = xGetFracPosition(shiftMv);

    std::string ctuLineKey = xGenerateCtuLineKey(currFramePoc, yCU, refList);
    if(motionDataMap.find(ctuLineKey) != motionDataMap.end()) {
        motionDataMap.at(ctuLineKey).push_back(md);
    }
    else {
        std::list<MotionData*> list;
        list.push_back(md);
        motionDataMap.insert({ctuLineKey, list});
    } 
}

void EncoderOptimizer::reportMotionData() {
    std::cout << "#### REPORT ####\n";
    for(auto it = motionDataMap.begin(); it != motionDataMap.end(); ++it) {
        std::string ctuLineKey = it->first;
        int cusWithinLine = it->second.size();

        std::pair<int, double> resultPrefFrac = calculatePrefFrac(ctuLineKey);
        int prefFrac = resultPrefFrac.first;
        int prefFracHit = (int) (resultPrefFrac.second * 100.0);

        std::cout << ctuLineKey << ": " << cusWithinLine  << " | PrefFrac " << prefFrac << "[" << prefFracHit << "]" << std::endl;
    }
}