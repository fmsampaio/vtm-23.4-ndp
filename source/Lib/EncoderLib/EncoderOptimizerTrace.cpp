#include "EncoderOptimizerTrace.h"

void EncoderOptimizerTrace::traceCtuCodingInfo(const CodingStructure& cs, const UnitArea& ctuArea, Position ctuPos) {
    
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


// if( bSplitCS && chosenCS->area.Y().width == MAX_CU_SIZE && chosenCS->area.Y().height == MAX_CU_SIZE)
//   {  
//     DTRACE( g_trace_ctx, D_NDP_TRACE, "POC. %3d CTU: @(%4d,%4d) [%2dx%2d]\n",
//     chosenCS->slice->getPOC(),
//     chosenCS->area.Y().x, chosenCS->area.Y().y,
//     chosenCS->area.Y().width, chosenCS->area.Y().height
//     );
    
//     for(const CodingUnit &cu : chosenCS->traverseCUs( CS::getArea( *chosenCS, chosenCS->area, ChannelType::LUMA ), ChannelType::LUMA ) ) {
//       DTRACE( g_trace_ctx, D_NDP_TRACE, "CU: (%4d,%4d) [%2dx%2d] %s\n",
//         cu.lx(),  
//         cu.ly(),
//         cu.lwidth(),
//         cu.lheight(),
//         cu.predMode == PredMode::MODE_INTER ? "Inter" : "Intra"
//       );
//       if(cu.predMode == PredMode::MODE_INTER) {
//         for (auto &pu : CU::traversePUs(cu)) {
//           if(pu.cu->affine) {
//             DTRACE( g_trace_ctx, D_NDP_TRACE, "Affine!\n");
//           }
//           else {
//             if(pu.refIdx[REF_PIC_LIST_0] >= 0) {
//               Mv mvL0 = pu.mv[REF_PIC_LIST_0];
//               DTRACE( g_trace_ctx, D_NDP_TRACE, "L0: Ref. %3d (%d, %d) (%d, %d)\n",
//                 pu.cu->slice->getRefPic( REF_PIC_LIST_0, pu.refIdx[REF_PIC_LIST_0] )->getPOC(),
//                 mvL0.getHor(),
//                 mvL0.getVer(),
//                 mvL0.getHor() << 2,
//                 mvL0.getVer() << 2
//               );
//             }
//             if(pu.refIdx[REF_PIC_LIST_1] >= 0) {
//               Mv mvL1 = pu.mv[REF_PIC_LIST_1];
//               DTRACE( g_trace_ctx, D_NDP_TRACE, "L1: Ref. %3d (%d, %d) (%d, %d)\n",
//                 pu.cu->slice->getRefPic( REF_PIC_LIST_1, pu.refIdx[REF_PIC_LIST_1] )->getPOC(),
//                 mvL1.getHor(),
//                 mvL1.getVer(),
//                 mvL1.getHor() << 2,
//                 mvL1.getVer() << 2
//               );
//             }
//           }
//         }
//       }
//     }
//   }