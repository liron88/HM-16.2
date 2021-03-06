/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2014, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TEncCu.h
    \brief    Coding Unit (CU) encoder class (header)
*/

#ifndef __TENCCU__
#define __TENCCU__

// Include files
#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComYuv.h"
#include "TLibCommon/TComPrediction.h"
#include "TLibCommon/TComTrQuant.h"
#include "TLibCommon/TComBitCounter.h"
#include "TLibCommon/TComDataCU.h"

#include "TEncEntropy.h"
#include "TEncSearch.h"
#include "TEncRateCtrl.h"
//! \ingroup TLibEncoder
//! \{

class TEncTop;
class TEncSbac;
class TEncCavlc;
class TEncSlice;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// CU encoder class
class TEncCu
{
private:

  TComDataCU**            m_ppcBestCU;      ///< Best CUs in each depth
  TComDataCU**            m_ppcTempCU;      ///< Temporary CUs in each depth
  UChar                   m_uhTotalDepth;

  // SBD-related Variables
  UInt*					          m_uiAlphaDepths;  ///< Array of depths adopted by group alpha
  UInt*                   m_uiBetaDepths;   ///< Array of depths adopted by group beta
  Bool*                   m_bRangeDepths;   ///< Array of depths to evaluate in the current CTU
  Bool*                   m_bAdoptedByC;    ///< Array of depths adopted by CTU C. 
                                            ///< Used in Medium High similarity
  Bool*                   m_bAdoptedByColocated; ///< Array of depths adopted by Colocated CTU
                                                 ///< Used in Low similarity
  UInt                    m_uiSizeAlpha;    ///< Size of group alpha for the current CTU 
                                            ///< Used in Medium Low similarity

  // RRSP-related Variables
  Bool*                   m_bRRSPAdoptedDepths64x64ByA;      ///< Array of CTUs in group A that adopt 64x64
  Bool*                   m_bRRSPAdoptedDepths64x64ByB;      ///< Array of CTUs in group B that adopt 64x64
  UInt*                   m_uiRRSPAlphaReducedAdoptedDepths; ///< Array of depths adopted by CUs in the reduced region of group alpha
  UInt*                   m_uiRRSPBetaReducedAdoptedDepths;  ///< Array of depths adopted by CUs in the reduced region of group alpha
  UInt*                   m_uiRRSPGrandfatherAdoptedDepths;  ///< Array of depths adopted by CUs in the colocated 32x32 CU of the colocated 32x32 CU
  Bool*                   m_bReducedRangeDepths;             ///< Array of depths to evaluate in the current reduced region
  UInt                    m_RRSPNumOfCTUsInA;                ///< How many CTUs are included in group A for the current CTU
  UInt                    m_RRSPNumOfCTUsInB;                ///< How many CTUs are included in group B for the current CTU


  TComYuv**               m_ppcPredYuvBest; ///< Best Prediction Yuv for each depth
  TComYuv**               m_ppcResiYuvBest; ///< Best Residual Yuv for each depth
  TComYuv**               m_ppcRecoYuvBest; ///< Best Reconstruction Yuv for each depth
  TComYuv**               m_ppcPredYuvTemp; ///< Temporary Prediction Yuv for each depth
  TComYuv**               m_ppcResiYuvTemp; ///< Temporary Residual Yuv for each depth
  TComYuv**               m_ppcRecoYuvTemp; ///< Temporary Reconstruction Yuv for each depth
  TComYuv**               m_ppcOrigYuv;     ///< Original Yuv for each depth

  //  Data : encoder control
  Bool                    m_bEncodeDQP;
  Bool                    m_CodeChromaQpAdjFlag;
  Int                     m_ChromaQpAdjIdc;

  //  Access channel
  TEncCfg*                m_pcEncCfg;
  TEncSearch*             m_pcPredSearch;
  TComTrQuant*            m_pcTrQuant;
  TComRdCost*             m_pcRdCost;

  TEncEntropy*            m_pcEntropyCoder;
  TEncBinCABAC*           m_pcBinCABAC;

  // SBAC RD
  TEncSbac***             m_pppcRDSbacCoder;
  TEncSbac*               m_pcRDGoOnSbacCoder;
  TEncRateCtrl*           m_pcRateCtrl;

public:
  /// copy parameters from encoder class
  Void  init                ( TEncTop* pcEncTop );

  /// create internal buffers
  Void  create              ( UChar uhTotalDepth, UInt iMaxWidth, UInt iMaxHeight, ChromaFormat chromaFormat );

  /// destroy internal buffers
  Void  destroy             ();

  /// CTU analysis function
  Void  compressCtu         ( TComDataCU*  pCtu );

  /// CTU encoding function
  Void  encodeCtu           ( TComDataCU*  pCtu );

  Int   updateCtuDataISlice ( TComDataCU* pCtu, Int width, Int height );

protected:
  Void  finishCU            ( TComDataCU*  pcCU, UInt uiAbsPartIdx,           UInt uiDepth        );
#if AMP_ENC_SPEEDUP
  Void  xCompressCU         ( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth DEBUG_STRING_FN_DECLARE(sDebug), PartSize eParentPartSize = NUMBER_OF_PART_SIZES );
#else
  Void  xCompressCU         ( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth        );
#endif
  Void  xEncodeCU           ( TComDataCU*  pcCU, UInt uiAbsPartIdx,           UInt uiDepth        );

  Int   xComputeQP          ( TComDataCU* pcCU, UInt uiDepth );
  Void  xCheckBestMode      ( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth DEBUG_STRING_FN_DECLARE(sParent) DEBUG_STRING_FN_DECLARE(sTest) DEBUG_STRING_PASS_INTO(Bool bAddSizeInfo=true));

  Void  xCheckRDCostMerge2Nx2N( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU DEBUG_STRING_FN_DECLARE(sDebug), Bool *earlyDetectionSkipMode );

#if AMP_MRG
  Void  xCheckRDCostInter   ( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize DEBUG_STRING_FN_DECLARE(sDebug), Bool bUseMRG = false  );
#else
  Void  xCheckRDCostInter   ( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize  );
#endif

  Void  xCheckRDCostIntra   ( TComDataCU *&rpcBestCU,
                              TComDataCU *&rpcTempCU,
                              Double      &cost,
                              PartSize     ePartSize
                              DEBUG_STRING_FN_DECLARE(sDebug)
                            );

  Void  xCheckDQP           ( TComDataCU*  pcCU );

  Void  xCheckIntraPCM      ( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU                      );
  Void  xCopyAMVPInfo       ( AMVPInfo* pSrc, AMVPInfo* pDst );
  Void  xCopyYuv2Pic        (TComPic* rpcPic, UInt uiCUAddr, UInt uiAbsPartIdx, UInt uiDepth, UInt uiSrcDepth, TComDataCU* pcCU, UInt uiLPelX, UInt uiTPelY );
  Void  xCopyYuv2Tmp        ( UInt uhPartUnitIdx, UInt uiDepth );

  Bool getdQPFlag           ()                        { return m_bEncodeDQP;        }
  Void setdQPFlag           ( Bool b )                { m_bEncodeDQP = b;           }

  Bool getCodeChromaQpAdjFlag() { return m_CodeChromaQpAdjFlag; }
  Void setCodeChromaQpAdjFlag( Bool b ) { m_CodeChromaQpAdjFlag = b; }

#if ADAPTIVE_QP_SELECTION
  // Adaptive reconstruction level (ARL) statistics collection functions
  Void xCtuCollectARLStats(TComDataCU* pCtu);
  Int  xTuCollectARLStats(TCoeff* rpcCoeff, TCoeff* rpcArlCoeff, Int NumCoeffInCU, Double* cSum, UInt* numSamples );
#endif

#if AMP_ENC_SPEEDUP
#if AMP_MRG
  Void deriveTestModeAMP (TComDataCU *pcBestCU, PartSize eParentPartSize, Bool &bTestAMP_Hor, Bool &bTestAMP_Ver, Bool &bTestMergeAMP_Hor, Bool &bTestMergeAMP_Ver);
#else
  Void deriveTestModeAMP (TComDataCU *pcBestCU, PartSize eParentPartSize, Bool &bTestAMP_Hor, Bool &bTestAMP_Ver);
#endif
#endif

  Void  xFillPCMBuffer     ( TComDataCU* pCU, TComYuv* pOrgYuv );

  // SBD-related Functions
  Void  getAdoptedDepthsLeft      (TComDataCU* pcCU, Bool* bDepths, UInt R);
  Void  getAdoptedDepthsAbove     (TComDataCU* pcCU, Bool* bDepths, UInt R);
  Void  getAdoptedDepthsAboveLeft (TComDataCU* pcCU, Bool* bDepths, UInt R);
  Void  getAdoptedDepthsAboveRight(TComDataCU* pcCU, Bool* bDepths, UInt R);
  Void  getAdoptedDepthsRight     (TComDataCU* pcCU, Bool* bDepths, UInt R);
  Void  getAdoptedDepthsBottom    (TComDataCU* pcCU, Bool* bDepths, UInt R);
  Void  getAdoptedDepthsColocated (TComDataCU* pcCU, Bool* bDepths, UInt R);

  Void  initGroupAlpha();
  Void  updateGroupAlpha(Bool* bDepths);

  // builds the array of depths that are adopted by group alpha
  Void  buildGroupAlpha(TComDataCU* pcCU);

  Void  initGroupBeta();
  Void  updateGroupBeta(Bool* bDepths);

  // builds the array of depths that are adopted by the extended group beta
  Void  buildGroupBeta(TComDataCU* pcCU);

  Bool  isBetaIncludedInAlpha();

  // calculate and return the similarity level as explained in the article by R. Fan
  UInt  getSimLevel();

  // medium high similarity - returns if a certain depth is only adopted by CTU C in alpha and beta
  Bool  isOnlyAdoptedbyC();

  Void  initRangeDepths();

  Void  performHighSim(TComDataCU* pcCU);
  Void  performMediumHighSim(TComDataCU* pcCU);
  Void  performMediumLowSim();
  Void  performLowSim();

  // RRSP-related Functions
  Void  evaluateGroupA64x64   (TComDataCU* pcCU);
  Void  evaluateGroupB64x64   (TComDataCU* pcCU);
  UInt  getNumOf32x32CUsInA   (TComDataCU* pcCU);
  Void  buildRRSPAlphaGroup   (TComDataCU* pcCU, RRSP32x32CU uiPartUnitIdx);
  Void  buildRRSPBetaGroup    (TComDataCU* pcCU, RRSP32x32CU uiPartUnitIdx);
  UInt  getRRSPSimLevel       ();
  Void  setReducedRangeDepths (UInt simLevel, TComDataCU* pcCU, RRSP32x32CU uiPartUnitIdx);
  Void  performRRSPLowSim     (TComDataCU* pcCU, RRSP32x32CU uiPartUnitIdx);
  Void  performRRSPMediumSim  (TComDataCU* pcCU, RRSP32x32CU uiPartUnitIdx);
  Void  performRRSPHighSim    (TComDataCU* pcCU, RRSP32x32CU uiPartUnitIdx);
};

//! \}

#endif // __TENCMB__
