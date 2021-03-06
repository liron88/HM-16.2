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

/** \file     TEncCu.cpp
    \brief    Coding Unit (CU) encoder class
*/

#include <stdio.h>
#include "TEncTop.h"
#include "TEncCu.h"
#include "TEncAnalyze.h"
#include "TLibCommon/Debug.h"

#include <cmath>
#include <algorithm>
using namespace std;


//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

/**
 \param    uiTotalDepth  total number of allowable depth
 \param    uiMaxWidth    largest CU width
 \param    uiMaxHeight   largest CU height
 */
Void TEncCu::create(UChar uhTotalDepth, UInt uiMaxWidth, UInt uiMaxHeight, ChromaFormat chromaFormat)
{
  Int i;

  m_uhTotalDepth   = uhTotalDepth + 1;
  m_ppcBestCU      = new TComDataCU*[m_uhTotalDepth-1];
  m_ppcTempCU      = new TComDataCU*[m_uhTotalDepth-1];

  m_uiAlphaDepths       = new UInt[m_uhTotalDepth - 1];
  m_uiBetaDepths        = new UInt[m_uhTotalDepth - 1];
  m_bRangeDepths        = new Bool[m_uhTotalDepth - 1];
  m_bAdoptedByC         = new Bool[m_uhTotalDepth - 1];
  m_bAdoptedByColocated = new Bool[m_uhTotalDepth - 1];
  m_uiSizeAlpha         = 0;
  
  // m_bRRSPAdoptedDepths64x64ByA structure
  // *---*---*---*---* 
  // | A | B | C | I |
  // *---*---*---*---*
  m_bRRSPAdoptedDepths64x64ByA      = new Bool[4];
  // m_bRRSPAdoptedDepths64x64ByB structure
  // *---*---*---*---*---* 
  // | D | E | F | G | H |
  // *---*---*---*---*---*
  m_bRRSPAdoptedDepths64x64ByB      = new Bool[5];
  m_uiRRSPAlphaReducedAdoptedDepths = new UInt[m_uhTotalDepth - 2];
  m_uiRRSPBetaReducedAdoptedDepths  = new UInt[m_uhTotalDepth - 2];
  m_uiRRSPGrandfatherAdoptedDepths  = new UInt[m_uhTotalDepth - 2];
  m_bReducedRangeDepths             = new Bool[m_uhTotalDepth - 2];
  m_RRSPNumOfCTUsInA                = 0;
  m_RRSPNumOfCTUsInB                = 0;

  m_ppcPredYuvBest = new TComYuv*[m_uhTotalDepth-1];
  m_ppcResiYuvBest = new TComYuv*[m_uhTotalDepth-1];
  m_ppcRecoYuvBest = new TComYuv*[m_uhTotalDepth-1];
  m_ppcPredYuvTemp = new TComYuv*[m_uhTotalDepth-1];
  m_ppcResiYuvTemp = new TComYuv*[m_uhTotalDepth-1];
  m_ppcRecoYuvTemp = new TComYuv*[m_uhTotalDepth-1];
  m_ppcOrigYuv     = new TComYuv*[m_uhTotalDepth-1];

  UInt uiNumPartitions;
  for( i=0 ; i<m_uhTotalDepth-1 ; i++)
  {
    uiNumPartitions = 1<<( ( m_uhTotalDepth - i - 1 )<<1 );
    UInt uiWidth  = uiMaxWidth  >> i;
    UInt uiHeight = uiMaxHeight >> i;

    m_ppcBestCU[i] = new TComDataCU; m_ppcBestCU[i]->create( chromaFormat, uiNumPartitions, uiWidth, uiHeight, false, uiMaxWidth >> (m_uhTotalDepth - 1) );
    m_ppcTempCU[i] = new TComDataCU; m_ppcTempCU[i]->create( chromaFormat, uiNumPartitions, uiWidth, uiHeight, false, uiMaxWidth >> (m_uhTotalDepth - 1) );

    m_ppcPredYuvBest[i] = new TComYuv; m_ppcPredYuvBest[i]->create(uiWidth, uiHeight, chromaFormat);
    m_ppcResiYuvBest[i] = new TComYuv; m_ppcResiYuvBest[i]->create(uiWidth, uiHeight, chromaFormat);
    m_ppcRecoYuvBest[i] = new TComYuv; m_ppcRecoYuvBest[i]->create(uiWidth, uiHeight, chromaFormat);

    m_ppcPredYuvTemp[i] = new TComYuv; m_ppcPredYuvTemp[i]->create(uiWidth, uiHeight, chromaFormat);
    m_ppcResiYuvTemp[i] = new TComYuv; m_ppcResiYuvTemp[i]->create(uiWidth, uiHeight, chromaFormat);
    m_ppcRecoYuvTemp[i] = new TComYuv; m_ppcRecoYuvTemp[i]->create(uiWidth, uiHeight, chromaFormat);

    m_ppcOrigYuv    [i] = new TComYuv; m_ppcOrigYuv    [i]->create(uiWidth, uiHeight, chromaFormat);
  }

  m_bEncodeDQP          = false;
  m_CodeChromaQpAdjFlag = false;
  m_ChromaQpAdjIdc      = 0;

  // initialize partition order.
  UInt* piTmp = &g_auiZscanToRaster[0];
  initZscanToRaster( m_uhTotalDepth, 1, 0, piTmp);
  initRasterToZscan( uiMaxWidth, uiMaxHeight, m_uhTotalDepth );

  // initialize conversion matrix from partition index to pel
  initRasterToPelXY( uiMaxWidth, uiMaxHeight, m_uhTotalDepth );
}

Void TEncCu::destroy()
{
  Int i;

  for( i=0 ; i<m_uhTotalDepth-1 ; i++)
  {
    if(m_ppcBestCU[i])
    {
      m_ppcBestCU[i]->destroy();      delete m_ppcBestCU[i];      m_ppcBestCU[i] = NULL;
    }
    if(m_ppcTempCU[i])
    {
      m_ppcTempCU[i]->destroy();      delete m_ppcTempCU[i];      m_ppcTempCU[i] = NULL;
    }
    if(m_ppcPredYuvBest[i])
    {
      m_ppcPredYuvBest[i]->destroy(); delete m_ppcPredYuvBest[i]; m_ppcPredYuvBest[i] = NULL;
    }
    if(m_ppcResiYuvBest[i])
    {
      m_ppcResiYuvBest[i]->destroy(); delete m_ppcResiYuvBest[i]; m_ppcResiYuvBest[i] = NULL;
    }
    if(m_ppcRecoYuvBest[i])
    {
      m_ppcRecoYuvBest[i]->destroy(); delete m_ppcRecoYuvBest[i]; m_ppcRecoYuvBest[i] = NULL;
    }
    if(m_ppcPredYuvTemp[i])
    {
      m_ppcPredYuvTemp[i]->destroy(); delete m_ppcPredYuvTemp[i]; m_ppcPredYuvTemp[i] = NULL;
    }
    if(m_ppcResiYuvTemp[i])
    {
      m_ppcResiYuvTemp[i]->destroy(); delete m_ppcResiYuvTemp[i]; m_ppcResiYuvTemp[i] = NULL;
    }
    if(m_ppcRecoYuvTemp[i])
    {
      m_ppcRecoYuvTemp[i]->destroy(); delete m_ppcRecoYuvTemp[i]; m_ppcRecoYuvTemp[i] = NULL;
    }
    if(m_ppcOrigYuv[i])
    {
      m_ppcOrigYuv[i]->destroy();     delete m_ppcOrigYuv[i];     m_ppcOrigYuv[i] = NULL;
    }
  }
  if(m_ppcBestCU)
  {
    delete [] m_ppcBestCU;
    m_ppcBestCU = NULL;
  }
  if(m_ppcTempCU)
  {
    delete [] m_ppcTempCU;
    m_ppcTempCU = NULL;
  }

  if (m_uiAlphaDepths)
  {
    delete[] m_uiAlphaDepths;
    m_uiAlphaDepths = NULL;
  }
  if (m_uiBetaDepths)
  {
    delete[] m_uiBetaDepths;
    m_uiBetaDepths = NULL;
  }
  if (m_bRangeDepths)
  {
    delete[] m_bRangeDepths;
    m_bRangeDepths = NULL;
  }
  if (m_bAdoptedByC)
  {
    delete[] m_bAdoptedByC;
    m_bAdoptedByC = NULL;
  }
  if (m_bAdoptedByColocated)
  {
    delete[] m_bAdoptedByColocated;
    m_bAdoptedByColocated = NULL;
  }
  // RRSP-related
  if (m_bRRSPAdoptedDepths64x64ByA)
  {
    delete[] m_bRRSPAdoptedDepths64x64ByA;
    m_bRRSPAdoptedDepths64x64ByA = NULL;
  }
  if (m_bRRSPAdoptedDepths64x64ByB)
  {
    delete[] m_bRRSPAdoptedDepths64x64ByB;
    m_bRRSPAdoptedDepths64x64ByB = NULL;
  }
  if (m_uiRRSPAlphaReducedAdoptedDepths)
  {
    delete[] m_uiRRSPAlphaReducedAdoptedDepths;
    m_uiRRSPAlphaReducedAdoptedDepths = NULL;
  }
  if (m_uiRRSPBetaReducedAdoptedDepths)
  {
    delete[] m_uiRRSPBetaReducedAdoptedDepths;
    m_uiRRSPBetaReducedAdoptedDepths = NULL;
  }
  if (m_uiRRSPGrandfatherAdoptedDepths)
  {
    delete[] m_uiRRSPGrandfatherAdoptedDepths;
    m_uiRRSPGrandfatherAdoptedDepths = NULL;
  }
  if (m_bReducedRangeDepths)
  {
    delete[] m_bReducedRangeDepths;
    m_bReducedRangeDepths = NULL;
  }

  if(m_ppcPredYuvBest)
  {
    delete [] m_ppcPredYuvBest;
    m_ppcPredYuvBest = NULL;
  }
  if(m_ppcResiYuvBest)
  {
    delete [] m_ppcResiYuvBest;
    m_ppcResiYuvBest = NULL;
  }
  if(m_ppcRecoYuvBest)
  {
    delete [] m_ppcRecoYuvBest;
    m_ppcRecoYuvBest = NULL;
  }
  if(m_ppcPredYuvTemp)
  {
    delete [] m_ppcPredYuvTemp;
    m_ppcPredYuvTemp = NULL;
  }
  if(m_ppcResiYuvTemp)
  {
    delete [] m_ppcResiYuvTemp;
    m_ppcResiYuvTemp = NULL;
  }
  if(m_ppcRecoYuvTemp)
  {
    delete [] m_ppcRecoYuvTemp;
    m_ppcRecoYuvTemp = NULL;
  }
  if(m_ppcOrigYuv)
  {
    delete [] m_ppcOrigYuv;
    m_ppcOrigYuv = NULL;
  }
}

/** \param    pcEncTop      pointer of encoder class
 */
Void TEncCu::init( TEncTop* pcEncTop )
{
  m_pcEncCfg           = pcEncTop;
  m_pcPredSearch       = pcEncTop->getPredSearch();
  m_pcTrQuant          = pcEncTop->getTrQuant();
  m_pcRdCost           = pcEncTop->getRdCost();

  m_pcEntropyCoder     = pcEncTop->getEntropyCoder();
  m_pcBinCABAC         = pcEncTop->getBinCABAC();

  m_pppcRDSbacCoder    = pcEncTop->getRDSbacCoder();
  m_pcRDGoOnSbacCoder  = pcEncTop->getRDGoOnSbacCoder();

  m_pcRateCtrl         = pcEncTop->getRateCtrl();
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/** \param  rpcCU pointer of CU data class
 */
Void TEncCu::compressCtu( TComDataCU* pCtu )
{
  // initialize CU data
  m_ppcBestCU[0]->initCtu( pCtu->getPic(), pCtu->getCtuRsAddr() );
  m_ppcTempCU[0]->initCtu( pCtu->getPic(), pCtu->getCtuRsAddr() );

  if ( pCtu->getSlice()->getSliceType() != I_SLICE && m_pcEncCfg->getUseSBD() )
    // Similiarity Based Decision turned on and not an intra frame
  {
    buildGroupAlpha(pCtu);
    initRangeDepths();

    UInt simLevel = getSimLevel();
    if (simLevel == 1) // high similarity
    {
      performHighSim(pCtu);
    }
    else if (simLevel == m_uhTotalDepth - 1) // low
    {
      performLowSim();
    }
    else if (simLevel == m_uhTotalDepth - 2) // medium-low
    {
      performMediumLowSim();
    }
    else // medium-high
    {
      performMediumHighSim(pCtu);
    }
  }

  // analysis of CU
  DEBUG_STRING_NEW(sDebug)

  xCompressCU( m_ppcBestCU[0], m_ppcTempCU[0], 0 DEBUG_STRING_PASS_INTO(sDebug) );
  DEBUG_STRING_OUTPUT(std::cout, sDebug)

#if ADAPTIVE_QP_SELECTION
  if( m_pcEncCfg->getUseAdaptQpSelect() )
  {
    if(pCtu->getSlice()->getSliceType()!=I_SLICE) //IIII
    {
      xCtuCollectARLStats( pCtu );
    }
  }
#endif
}
/** \param  pcCU  pointer of CU data class
 */
Void TEncCu::encodeCtu ( TComDataCU* pCtu )
{
  if ( pCtu->getSlice()->getPPS()->getUseDQP() )
  {
    setdQPFlag(true);
  }

  if ( pCtu->getSlice()->getUseChromaQpAdj() )
  {
    setCodeChromaQpAdjFlag(true);
  }

  // Encode CU data
  xEncodeCU( pCtu, 0, 0 );
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================
/** Derive small set of test modes for AMP encoder speed-up
 *\param   rpcBestCU
 *\param   eParentPartSize
 *\param   bTestAMP_Hor
 *\param   bTestAMP_Ver
 *\param   bTestMergeAMP_Hor
 *\param   bTestMergeAMP_Ver
 *\returns Void
*/
#if AMP_ENC_SPEEDUP
#if AMP_MRG
Void TEncCu::deriveTestModeAMP (TComDataCU *pcBestCU, PartSize eParentPartSize, Bool &bTestAMP_Hor, Bool &bTestAMP_Ver, Bool &bTestMergeAMP_Hor, Bool &bTestMergeAMP_Ver)
#else
Void TEncCu::deriveTestModeAMP (TComDataCU *pcBestCU, PartSize eParentPartSize, Bool &bTestAMP_Hor, Bool &bTestAMP_Ver)
#endif
{
  if ( pcBestCU->getPartitionSize(0) == SIZE_2NxN )
  {
    bTestAMP_Hor = true;
  }
  else if ( pcBestCU->getPartitionSize(0) == SIZE_Nx2N )
  {
    bTestAMP_Ver = true;
  }
  else if ( pcBestCU->getPartitionSize(0) == SIZE_2Nx2N && pcBestCU->getMergeFlag(0) == false && pcBestCU->isSkipped(0) == false )
  {
    bTestAMP_Hor = true;
    bTestAMP_Ver = true;
  }

#if AMP_MRG
  //! Utilizing the partition size of parent PU
  if ( eParentPartSize >= SIZE_2NxnU && eParentPartSize <= SIZE_nRx2N )
  {
    bTestMergeAMP_Hor = true;
    bTestMergeAMP_Ver = true;
  }

  if ( eParentPartSize == NUMBER_OF_PART_SIZES ) //! if parent is intra
  {
    if ( pcBestCU->getPartitionSize(0) == SIZE_2NxN )
    {
      bTestMergeAMP_Hor = true;
    }
    else if ( pcBestCU->getPartitionSize(0) == SIZE_Nx2N )
    {
      bTestMergeAMP_Ver = true;
    }
  }

  if ( pcBestCU->getPartitionSize(0) == SIZE_2Nx2N && pcBestCU->isSkipped(0) == false )
  {
    bTestMergeAMP_Hor = true;
    bTestMergeAMP_Ver = true;
  }

  if ( pcBestCU->getWidth(0) == 64 )
  {
    bTestAMP_Hor = false;
    bTestAMP_Ver = false;
  }
#else
  //! Utilizing the partition size of parent PU
  if ( eParentPartSize >= SIZE_2NxnU && eParentPartSize <= SIZE_nRx2N )
  {
    bTestAMP_Hor = true;
    bTestAMP_Ver = true;
  }

  if ( eParentPartSize == SIZE_2Nx2N )
  {
    bTestAMP_Hor = false;
    bTestAMP_Ver = false;
  }
#endif
}
#endif


// ====================================================================================================================
// Protected member functions
// ====================================================================================================================
/** Compress a CU block recursively with enabling sub-CTU-level delta QP
 *\param   rpcBestCU
 *\param   rpcTempCU
 *\param   uiDepth
 *\returns Void
 *
 *- for loop of QP value to compress the current CU with all possible QP
*/
#if AMP_ENC_SPEEDUP
Void TEncCu::xCompressCU( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth DEBUG_STRING_FN_DECLARE(sDebug_), PartSize eParentPartSize )
#else
Void TEncCu::xCompressCU( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth )
#endif
{
  TComPic* pcPic = rpcBestCU->getPic();
  DEBUG_STRING_NEW(sDebug)

  // get Original YUV data from picture
  m_ppcOrigYuv[uiDepth]->copyFromPicYuv( pcPic->getPicYuvOrg(), rpcBestCU->getCtuRsAddr(), rpcBestCU->getZorderIdxInCtu() );

  // variables for Reduced Region Similarity Partitioning (RRSP)
  UInt numOfAdoptedCTUsInA64x64 = 0;
  UInt numOfAdoptedCTUsInB64x64 = 0;
  Bool bOnlyDepth0 = false;
  Bool bCheck64x64 = true;

  if (m_pcEncCfg->getUseRRSP() && uiDepth == 0 && rpcBestCU->getSlice()->getSliceType() != I_SLICE)
  {
    evaluateGroupA64x64(rpcBestCU);
    for (UInt ui = 0; ui < 4; ui++)
    {
      numOfAdoptedCTUsInA64x64 += (m_bRRSPAdoptedDepths64x64ByA[ui] == true);
    }
    if (numOfAdoptedCTUsInA64x64 == m_RRSPNumOfCTUsInA)
    {
      numOfAdoptedCTUsInB64x64 = 0;
      evaluateGroupB64x64(rpcBestCU);
      for (UInt ui = 0; ui < 5; ui++)
      {
        numOfAdoptedCTUsInB64x64 += (m_bRRSPAdoptedDepths64x64ByB[ui] == true);
      }
      if (numOfAdoptedCTUsInB64x64 == m_RRSPNumOfCTUsInB)
      {
        Int iRRSPQP = rpcBestCU->getQP(0);
        if (m_pcEncCfg->getUseRateCtrl())
        {
          iRRSPQP = m_pcRateCtrl->getRCQP();
        }
        if (iRRSPQP > 35)
          // higher QPs tend to adopt the same CU sizes
        {
          bOnlyDepth0 = true;
        }
        else if (rpcBestCU->getCUColocated(REF_PIC_LIST_0)->getCUColocated(REF_PIC_LIST_0) == NULL)
        {
          bOnlyDepth0 = true;
        }
        else if (rpcBestCU->getCUColocated(REF_PIC_LIST_0)->getCUColocated(REF_PIC_LIST_0)->getDepth(0) == 0)
        {
          bOnlyDepth0 = true;
        }
      }
    }
    bCheck64x64 = (numOfAdoptedCTUsInA64x64 > 0 || ((rpcBestCU->getCUColocated(REF_PIC_LIST_0)->getSlice()->getSliceType() == I_SLICE) && (rpcBestCU->getCUPelY() < 32)));
    if (!bCheck64x64)
    {
      evaluateGroupB64x64(rpcBestCU);
      for (UInt ui = 0; ui < 5; ui++)
      {
        numOfAdoptedCTUsInB64x64 += (m_bRRSPAdoptedDepths64x64ByB[ui] == true);
      }
      bCheck64x64 = numOfAdoptedCTUsInB64x64 > 0;
      if (!bCheck64x64)
      {
        UInt uiNumOf32x32CUsInA = getNumOf32x32CUsInA(rpcBestCU);
        // similar smaller CUs tend to combine into a larger CU
        bCheck64x64 = uiNumOf32x32CUsInA >= (m_RRSPNumOfCTUsInA * NUM_OF_32X32_CTUS_IN_64X64_CTU / 2);
      }
    }
  }

  // variable for Early CU determination
  Bool    bSubBranch = true;

  // variable for Cbf fast mode PU decision
  Bool    doNotBlockPu = true;
  Bool    earlyDetectionSkipMode = false;

  Bool bBoundary = false;
  UInt uiLPelX   = rpcBestCU->getCUPelX();
  UInt uiRPelX   = uiLPelX + rpcBestCU->getWidth(0)  - 1;
  UInt uiTPelY   = rpcBestCU->getCUPelY();
  UInt uiBPelY   = uiTPelY + rpcBestCU->getHeight(0) - 1;

  Int iBaseQP = xComputeQP( rpcBestCU, uiDepth );
  Int iMinQP;
  Int iMaxQP;
  Bool isAddLowestQP = false;

  const UInt numberValidComponents = rpcBestCU->getPic()->getNumberValidComponents();

  if( (g_uiMaxCUWidth>>uiDepth) >= rpcTempCU->getSlice()->getPPS()->getMinCuDQPSize() )
  {
    Int idQP = m_pcEncCfg->getMaxDeltaQP();
    iMinQP = Clip3( -rpcTempCU->getSlice()->getSPS()->getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP-idQP );
    iMaxQP = Clip3( -rpcTempCU->getSlice()->getSPS()->getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP+idQP );
  }
  else
  {
    iMinQP = rpcTempCU->getQP(0);
    iMaxQP = rpcTempCU->getQP(0);
  }

  if ( m_pcEncCfg->getUseRateCtrl() )
  {
    iMinQP = m_pcRateCtrl->getRCQP();
    iMaxQP = m_pcRateCtrl->getRCQP();
  }

  // transquant-bypass (TQB) processing loop variable initialisation ---

  const Int lowestQP = iMinQP; // For TQB, use this QP which is the lowest non TQB QP tested (rather than QP'=0) - that way delta QPs are smaller, and TQB can be tested at all CU levels.

  if ( (rpcTempCU->getSlice()->getPPS()->getTransquantBypassEnableFlag()) )
  {
    isAddLowestQP = true; // mark that the first iteration is to cost TQB mode.
    iMinQP = iMinQP - 1;  // increase loop variable range by 1, to allow bOnlyDepth0ng of TQB mode along with other QPs
    if ( m_pcEncCfg->getCUTransquantBypassFlagForceValue() )
    {
      iMaxQP = iMinQP;
    }
  }

  TComSlice * pcSlice = rpcTempCU->getPic()->getSlice(rpcTempCU->getPic()->getCurrSliceIdx());
  // We need to split, so don't try these modes.
  if ( ( uiRPelX < rpcBestCU->getSlice()->getSPS()->getPicWidthInLumaSamples() ) &&
       ( uiBPelY < rpcBestCU->getSlice()->getSPS()->getPicHeightInLumaSamples() ) )
  {
    // variable for Similarity Based Decision by R. Fan
    Bool bSBD  = (!m_pcEncCfg->getUseSBD() || (m_pcEncCfg->getUseSBD() && m_bRangeDepths[uiDepth] && rpcBestCU->getSlice()->getSliceType() != I_SLICE) || rpcBestCU->getSlice()->getSliceType() == I_SLICE) ? true : false;
    // variable for Reduced Region Similarity Partitioning (RRSP)
    Bool bRRSP = (!m_pcEncCfg->getUseRRSP() || (m_pcEncCfg->getUseRRSP() && ((uiDepth == 0 && bCheck64x64) || (uiDepth > 0 && m_bReducedRangeDepths[uiDepth - 1] == true)) && rpcBestCU->getSlice()->getSliceType() != I_SLICE) || rpcBestCU->getSlice()->getSliceType() == I_SLICE) ? true : false;
    
    if (bSBD && bRRSP)
      // Similarity Based Decision is turned on, perform required inter/intra/SKIP modes
    {
      for (Int iQP = iMinQP; iQP <= iMaxQP; iQP++)
      {
        const Bool bIsLosslessMode = isAddLowestQP && (iQP == iMinQP);

        if (bIsLosslessMode)
        {
          iQP = lowestQP;
        }

        m_ChromaQpAdjIdc = 0;
        if (pcSlice->getUseChromaQpAdj())
        {
          /* Pre-estimation of chroma QP based on input block activity may be performed
           * here, using for example m_ppcOrigYuv[uiDepth] */
          /* To exercise the current code, the index used for adjustment is based on
           * block position
           */
          Int lgMinCuSize = pcSlice->getSPS()->getLog2MinCodingBlockSize();
          m_ChromaQpAdjIdc = ((uiLPelX >> lgMinCuSize) + (uiTPelY >> lgMinCuSize)) % (pcSlice->getPPS()->getChromaQpAdjTableSize() + 1);
        }

        rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);

        // do inter modes, SKIP and 2Nx2N
        if (rpcBestCU->getSlice()->getSliceType() != I_SLICE)
        {
          // 2Nx2N
          if (m_pcEncCfg->getUseEarlySkipDetection())
          {
            xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2Nx2N DEBUG_STRING_PASS_INTO(sDebug));
            rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);//by Competition for inter_2Nx2N
          }
          // SKIP
          xCheckRDCostMerge2Nx2N(rpcBestCU, rpcTempCU DEBUG_STRING_PASS_INTO(sDebug), &earlyDetectionSkipMode);//by Merge for inter_2Nx2N
          rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);

          if (!m_pcEncCfg->getUseEarlySkipDetection())
          {
            // 2Nx2N, NxN
            xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2Nx2N DEBUG_STRING_PASS_INTO(sDebug));
            rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
            if (m_pcEncCfg->getUseCbfFastMode())
            {
              doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
            }
          }
        }

        if (bIsLosslessMode) // Restore loop variable if lossless mode was searched.
        {
          iQP = iMinQP;
        }
      }

      if (!earlyDetectionSkipMode)
      {
        for (Int iQP = iMinQP; iQP <= iMaxQP; iQP++)
        {
          const Bool bIsLosslessMode = isAddLowestQP && (iQP == iMinQP); // If lossless, then iQP is irrelevant for subsequent modules.

          if (bIsLosslessMode)
          {
            iQP = lowestQP;
          }

          rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);

          // do inter modes, NxN, 2NxN, and Nx2N
          if (rpcBestCU->getSlice()->getSliceType() != I_SLICE)
          {
            // 2Nx2N, NxN
            if (!((rpcBestCU->getWidth(0) == 8) && (rpcBestCU->getHeight(0) == 8)))
            {
              if (uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth && doNotBlockPu)
              {
                xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_NxN DEBUG_STRING_PASS_INTO(sDebug));
                rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
              }
            }

            if (doNotBlockPu)
            {
              xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_Nx2N DEBUG_STRING_PASS_INTO(sDebug));
              rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
              if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_Nx2N)
              {
                doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
              }
            }
            if (doNotBlockPu)
            {
              xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2NxN DEBUG_STRING_PASS_INTO(sDebug));
              rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
              if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxN)
              {
                doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
              }
            }

            //! Try AMP (SIZE_2NxnU, SIZE_2NxnD, SIZE_nLx2N, SIZE_nRx2N)
            if (pcPic->getSlice(0)->getSPS()->getAMPAcc(uiDepth))
            {
#if AMP_ENC_SPEEDUP
              Bool bTestAMP_Hor = false, bTestAMP_Ver = false;

#if AMP_MRG
              Bool bTestMergeAMP_Hor = false, bTestMergeAMP_Ver = false;

              deriveTestModeAMP(rpcBestCU, eParentPartSize, bTestAMP_Hor, bTestAMP_Ver, bTestMergeAMP_Hor, bTestMergeAMP_Ver);
#else
              deriveTestModeAMP (rpcBestCU, eParentPartSize, bTestAMP_Hor, bTestAMP_Ver);
#endif

              //! Do horizontal AMP
              if (bTestAMP_Hor)
              {
                if (doNotBlockPu)
                {
                  xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2NxnU DEBUG_STRING_PASS_INTO(sDebug));
                  rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
                  if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnU)
                  {
                    doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
                  }
                }
                if (doNotBlockPu)
                {
                  xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2NxnD DEBUG_STRING_PASS_INTO(sDebug));
                  rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
                  if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnD)
                  {
                    doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
                  }
                }
              }
#if AMP_MRG
              else if (bTestMergeAMP_Hor)
              {
                if (doNotBlockPu)
                {
                  xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2NxnU DEBUG_STRING_PASS_INTO(sDebug), true);
                  rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
                  if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnU)
                  {
                    doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
                  }
                }
                if (doNotBlockPu)
                {
                  xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2NxnD DEBUG_STRING_PASS_INTO(sDebug), true);
                  rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
                  if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnD)
                  {
                    doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
                  }
                }
              }
#endif

              //! Do horizontal AMP
              if (bTestAMP_Ver)
              {
                if (doNotBlockPu)
                {
                  xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_nLx2N DEBUG_STRING_PASS_INTO(sDebug));
                  rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
                  if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_nLx2N)
                  {
                    doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
                  }
                }
                if (doNotBlockPu)
                {
                  xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_nRx2N DEBUG_STRING_PASS_INTO(sDebug));
                  rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
                }
              }
#if AMP_MRG
              else if (bTestMergeAMP_Ver)
              {
                if (doNotBlockPu)
                {
                  xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_nLx2N DEBUG_STRING_PASS_INTO(sDebug), true);
                  rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
                  if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_nLx2N)
                  {
                    doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
                  }
                }
                if (doNotBlockPu)
                {
                  xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_nRx2N DEBUG_STRING_PASS_INTO(sDebug), true);
                  rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
                }
              }
#endif

#else
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU );
              rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD );
              rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N );
              rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );

              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N );
              rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );

#endif
            }
          }

          // do normal intra modes
          // speedup for inter frames
          Double intraCost = 0.0;

          if ((rpcBestCU->getSlice()->getSliceType() == I_SLICE) ||
            (rpcBestCU->getCbf(0, COMPONENT_Y) != 0) ||
            ((rpcBestCU->getCbf(0, COMPONENT_Cb) != 0) && (numberValidComponents > COMPONENT_Cb)) ||
            ((rpcBestCU->getCbf(0, COMPONENT_Cr) != 0) && (numberValidComponents > COMPONENT_Cr))) // avoid very complex intra if it is unlikely
          {
            xCheckRDCostIntra(rpcBestCU, rpcTempCU, intraCost, SIZE_2Nx2N DEBUG_STRING_PASS_INTO(sDebug));
            rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
            if (uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth)
            {
              if (rpcTempCU->getWidth(0) > (1 << rpcTempCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize()))
              {
                Double tmpIntraCost;
                xCheckRDCostIntra(rpcBestCU, rpcTempCU, tmpIntraCost, SIZE_NxN DEBUG_STRING_PASS_INTO(sDebug));
                intraCost = std::min(intraCost, tmpIntraCost);
                rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
              }
            }
          }

          // test PCM
          if (pcPic->getSlice(0)->getSPS()->getUsePCM()
            && rpcTempCU->getWidth(0) <= (1 << pcPic->getSlice(0)->getSPS()->getPCMLog2MaxSize())
            && rpcTempCU->getWidth(0) >= (1 << pcPic->getSlice(0)->getSPS()->getPCMLog2MinSize()))
          {
            UInt uiRawBits = getTotalBits(rpcBestCU->getWidth(0), rpcBestCU->getHeight(0), rpcBestCU->getPic()->getChromaFormat(), g_bitDepth);
            UInt uiBestBits = rpcBestCU->getTotalBits();
            if ((uiBestBits > uiRawBits) || (rpcBestCU->getTotalCost() > m_pcRdCost->calcRdCost(uiRawBits, 0)))
            {
              xCheckIntraPCM(rpcBestCU, rpcTempCU);
              rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
            }
          }

          if (bIsLosslessMode) // Restore loop variable if lossless mode was searched.
          {
            iQP = iMinQP;
          }
        }
      }

      m_pcEntropyCoder->resetBits();
      m_pcEntropyCoder->encodeSplitFlag(rpcBestCU, 0, uiDepth, true);
      rpcBestCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // split bits
      rpcBestCU->getTotalBins() += ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
      rpcBestCU->getTotalCost() = m_pcRdCost->calcRdCost(rpcBestCU->getTotalBits(), rpcBestCU->getTotalDistortion());
    }

    // Early CU determination
    if( m_pcEncCfg->getUseEarlyCU() && rpcBestCU->isSkipped(0) )
    {
      bSubBranch = false;
    }
    else
    {
      bSubBranch = true;
    }
  }
  else
  {
    bBoundary = true;
  }

  // copy orginal YUV samples to PCM buffer
  if( rpcBestCU->isLosslessCoded(0) && (rpcBestCU->getIPCMFlag(0) == false))
  {
    xFillPCMBuffer(rpcBestCU, m_ppcOrigYuv[uiDepth]);
  }

  if( (g_uiMaxCUWidth>>uiDepth) == rpcTempCU->getSlice()->getPPS()->getMinCuDQPSize() )
  {
    Int idQP = m_pcEncCfg->getMaxDeltaQP();
    iMinQP = Clip3( -rpcTempCU->getSlice()->getSPS()->getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP-idQP );
    iMaxQP = Clip3( -rpcTempCU->getSlice()->getSPS()->getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP+idQP );
  }
  else if( (g_uiMaxCUWidth>>uiDepth) > rpcTempCU->getSlice()->getPPS()->getMinCuDQPSize() )
  {
    iMinQP = iBaseQP;
    iMaxQP = iBaseQP;
  }
  else
  {
    const Int iStartQP = rpcTempCU->getQP(0);
    iMinQP = iStartQP;
    iMaxQP = iStartQP;
  }

  if ( m_pcEncCfg->getUseRateCtrl() )
  {
    iMinQP = m_pcRateCtrl->getRCQP();
    iMaxQP = m_pcRateCtrl->getRCQP();
  }

  if ( m_pcEncCfg->getCUTransquantBypassFlagForceValue() )
  {
    iMaxQP = iMinQP; // If all TUs are forced into using transquant bypass, do not loop here.
  }

  // SBD - Once the maximal depth in the selected range is complete, stop splitting
  Bool bSBDSplit = true;
  if (m_pcEncCfg->getUseSBD() && rpcBestCU->getSlice()->getSliceType() != I_SLICE)
  {
    bSBDSplit = false;
    for (UInt ui = uiDepth + 1; ui <= g_uiMaxCUDepth - g_uiAddCUDepth; ui++)
    {
      if (m_bRangeDepths[ui] == true)
      {
        bSBDSplit = true;
        break;
      }
      if (bBoundary && (ui == g_uiMaxCUDepth - g_uiAddCUDepth))
        // CU is out of the frame boundaries, must select prediction mode for at least one of the next depths
      {
        bSBDSplit = true;
        m_bRangeDepths[uiDepth + 1] = true;
      }
    }
  }

  Bool bRRSPSplit = true;
  if (m_pcEncCfg->getUseRRSP() && rpcBestCU->getSlice()->getSliceType() != I_SLICE)
  {
    if (uiDepth == 0 && bOnlyDepth0)
    {
      // Move to next CTU
      bRRSPSplit = false;
      if (bBoundary)
        // CU is out of the frame boundaries, must select prediction mode for at least one of the next depths
      {
        bRRSPSplit = true;
        m_bReducedRangeDepths[uiDepth] = true;
      }
    }
    else if (uiDepth > 0)
    {
      for (UInt ui = uiDepth + 1; ui <= g_uiMaxCUDepth - g_uiAddCUDepth; ui++)
      {
        if (m_bReducedRangeDepths[ui - 1] == true)
        {
          break;
        }
        if (ui == g_uiMaxCUDepth - g_uiAddCUDepth)
        {
          bRRSPSplit = false;
          if (bBoundary)
            // CU is out of the frame boundaries, must select prediction mode for at least one of the next depths
          {
            bRRSPSplit = true;
            m_bReducedRangeDepths[uiDepth] = true;
          }
        }
      }
    }
  }

  for (Int iQP=iMinQP; iQP<=iMaxQP; iQP++)
  {
    const Bool bIsLosslessMode = false; // False at this level. Next level down may set it to true.

    rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );

    // further split
    if ( bRRSPSplit && bSBDSplit && bSubBranch && uiDepth < g_uiMaxCUDepth - g_uiAddCUDepth)
    {
      UChar       uhNextDepth         = uiDepth+1;
      TComDataCU* pcSubBestPartCU     = m_ppcBestCU[uhNextDepth];
      TComDataCU* pcSubTempPartCU     = m_ppcTempCU[uhNextDepth];
      DEBUG_STRING_NEW(sTempDebug)

      for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++ )
      {
        pcSubBestPartCU->initSubCU( rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP );           // clear sub partition datas or init.
        pcSubTempPartCU->initSubCU( rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP );           // clear sub partition datas or init.

        // RRSP-related algorithm
        if (m_pcEncCfg->getUseRRSP() && uiDepth == 0 && rpcBestCU->getSlice()->getSliceType() != I_SLICE)
        {
          buildRRSPAlphaGroup(rpcTempCU, (RRSP32x32CU)uiPartUnitIdx);
          setReducedRangeDepths(getRRSPSimLevel(), rpcTempCU, (RRSP32x32CU)uiPartUnitIdx);
          if ((rpcTempCU->getCUColocated(REF_PIC_LIST_0)->getSlice()->getSliceType() == I_SLICE) && (rpcTempCU->getCUPelY() < 32))
            // if the previous frame was an intra frame, which tends to adopt smaller CU sizes, also evaluate a CU size of 32x32 in the first 32x32 row
          {
            m_bReducedRangeDepths[0] = true;
          }
        }


        if( ( pcSubBestPartCU->getCUPelX() < pcSlice->getSPS()->getPicWidthInLumaSamples() ) && ( pcSubBestPartCU->getCUPelY() < pcSlice->getSPS()->getPicHeightInLumaSamples() ) )
        {
          if ( 0 == uiPartUnitIdx) //initialize RD with previous depth buffer
          {
            m_pppcRDSbacCoder[uhNextDepth][CI_CURR_BEST]->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);
          }
          else
          {
            m_pppcRDSbacCoder[uhNextDepth][CI_CURR_BEST]->load(m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]);
          }

#if AMP_ENC_SPEEDUP
          DEBUG_STRING_NEW(sChild)
          if ( !rpcBestCU->isInter(0) )
          {
            xCompressCU( pcSubBestPartCU, pcSubTempPartCU, uhNextDepth DEBUG_STRING_PASS_INTO(sChild), NUMBER_OF_PART_SIZES );
          }
          else
          {

            xCompressCU( pcSubBestPartCU, pcSubTempPartCU, uhNextDepth DEBUG_STRING_PASS_INTO(sChild), rpcBestCU->getPartitionSize(0) );
          }
          DEBUG_STRING_APPEND(sTempDebug, sChild)
#else
          xCompressCU( pcSubBestPartCU, pcSubTempPartCU, uhNextDepth );
#endif

          rpcTempCU->copyPartFrom( pcSubBestPartCU, uiPartUnitIdx, uhNextDepth );         // Keep best part data to current temporary data.
          xCopyYuv2Tmp( pcSubBestPartCU->getTotalNumPart()*uiPartUnitIdx, uhNextDepth );
        }
        else
        {
          pcSubBestPartCU->copyToPic( uhNextDepth );
          rpcTempCU->copyPartFrom( pcSubBestPartCU, uiPartUnitIdx, uhNextDepth );
        }
      }

      if( !bBoundary )
      {
        m_pcEntropyCoder->resetBits();
        m_pcEntropyCoder->encodeSplitFlag( rpcTempCU, 0, uiDepth, true );

        rpcTempCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // split bits
        rpcTempCU->getTotalBins() += ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
      }
      rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );

      if( (g_uiMaxCUWidth>>uiDepth) == rpcTempCU->getSlice()->getPPS()->getMinCuDQPSize() && rpcTempCU->getSlice()->getPPS()->getUseDQP())
      {
        Bool hasResidual = false;
        for( UInt uiBlkIdx = 0; uiBlkIdx < rpcTempCU->getTotalNumPart(); uiBlkIdx ++)
        {
          if( (     rpcTempCU->getCbf(uiBlkIdx, COMPONENT_Y)
                || (rpcTempCU->getCbf(uiBlkIdx, COMPONENT_Cb) && (numberValidComponents > COMPONENT_Cb))
                || (rpcTempCU->getCbf(uiBlkIdx, COMPONENT_Cr) && (numberValidComponents > COMPONENT_Cr)) ) )
          {
            hasResidual = true;
            break;
          }
        }

        UInt uiTargetPartIdx = 0;
        if ( hasResidual )
        {
#if !RDO_WITHOUT_DQP_BITS
          m_pcEntropyCoder->resetBits();
          m_pcEntropyCoder->encodeQP( rpcTempCU, uiTargetPartIdx, false );
          rpcTempCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // dQP bits
          rpcTempCU->getTotalBins() += ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
          rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
#endif

          Bool foundNonZeroCbf = false;
          rpcTempCU->setQPSubCUs( rpcTempCU->getRefQP( uiTargetPartIdx ), 0, uiDepth, foundNonZeroCbf );
          assert( foundNonZeroCbf );
        }
        else
        {
          rpcTempCU->setQPSubParts( rpcTempCU->getRefQP( uiTargetPartIdx ), 0, uiDepth ); // set QP to default QP
        }
      }

      m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);

      // TODO: this does not account for the slice bytes already written. See other instances of FIXED_NUMBER_OF_BYTES
      Bool isEndOfSlice        = rpcBestCU->getSlice()->getSliceMode()==FIXED_NUMBER_OF_BYTES
                                 && (rpcBestCU->getTotalBits()>rpcBestCU->getSlice()->getSliceArgument()<<3);
      Bool isEndOfSliceSegment = rpcBestCU->getSlice()->getSliceSegmentMode()==FIXED_NUMBER_OF_BYTES
                                 && (rpcBestCU->getTotalBits()>rpcBestCU->getSlice()->getSliceSegmentArgument()<<3);
      if(isEndOfSlice||isEndOfSliceSegment)
      {
        if (m_pcEncCfg->getCostMode()==COST_MIXED_LOSSLESS_LOSSY_CODING)
          rpcBestCU->getTotalCost()=rpcTempCU->getTotalCost() + (1.0 / m_pcRdCost->getLambda());
        else
          rpcBestCU->getTotalCost()=rpcTempCU->getTotalCost()+1;
      }

      xCheckBestMode( rpcBestCU, rpcTempCU, uiDepth DEBUG_STRING_PASS_INTO(sDebug) DEBUG_STRING_PASS_INTO(sTempDebug) DEBUG_STRING_PASS_INTO(false) ); // RD compare current larger prediction
                                                                                       // with sub partitioned prediction.
    }
  }

  DEBUG_STRING_APPEND(sDebug_, sDebug);

  rpcBestCU->copyToPic(uiDepth);                                                     // Copy Best data to Picture for next partition prediction.

  xCopyYuv2Pic( rpcBestCU->getPic(), rpcBestCU->getCtuRsAddr(), rpcBestCU->getZorderIdxInCtu(), uiDepth, uiDepth, rpcBestCU, uiLPelX, uiTPelY );   // Copy Yuv data to picture Yuv
  if (bBoundary)
  {
    return;
  }

  // Assert if Best prediction mode is NONE
  // Selected mode's RD-cost must be not MAX_DOUBLE.
  assert( rpcBestCU->getPartitionSize ( 0 ) != NUMBER_OF_PART_SIZES       );
  assert( rpcBestCU->getPredictionMode( 0 ) != NUMBER_OF_PREDICTION_MODES );
  assert( rpcBestCU->getTotalCost     (   ) != MAX_DOUBLE                 );
}

/** finish encoding a cu and handle end-of-slice conditions
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiDepth
 * \returns Void
 */
Void TEncCu::finishCU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  TComPic* pcPic = pcCU->getPic();
  TComSlice * pcSlice = pcCU->getPic()->getSlice(pcCU->getPic()->getCurrSliceIdx());

  //Calculate end address
  const Int  currentCTUTsAddr = pcPic->getPicSym()->getCtuRsToTsAddrMap(pcCU->getCtuRsAddr());
  const Bool isLastSubCUOfCtu = pcCU->isLastSubCUOfCtu(uiAbsPartIdx);
  if ( isLastSubCUOfCtu )
  {
    // The 1-terminating bit is added to all streams, so don't add it here when it's 1.
    // i.e. when the slice segment CurEnd CTU address is the current CTU address+1.
    if (pcSlice->getSliceSegmentCurEndCtuTsAddr() != currentCTUTsAddr+1)
    {
      m_pcEntropyCoder->encodeTerminatingBit( 0 );
    }
  }
}

/** Compute QP for each CU
 * \param pcCU Target CU
 * \param uiDepth CU depth
 * \returns quantization parameter
 */
Int TEncCu::xComputeQP( TComDataCU* pcCU, UInt uiDepth )
{
  Int iBaseQp = pcCU->getSlice()->getSliceQp();
  Int iQpOffset = 0;
  if ( m_pcEncCfg->getUseAdaptiveQP() )
  {
    TEncPic* pcEPic = dynamic_cast<TEncPic*>( pcCU->getPic() );
    UInt uiAQDepth = min( uiDepth, pcEPic->getMaxAQDepth()-1 );
    TEncPicQPAdaptationLayer* pcAQLayer = pcEPic->getAQLayer( uiAQDepth );
    UInt uiAQUPosX = pcCU->getCUPelX() / pcAQLayer->getAQPartWidth();
    UInt uiAQUPosY = pcCU->getCUPelY() / pcAQLayer->getAQPartHeight();
    UInt uiAQUStride = pcAQLayer->getAQPartStride();
    TEncQPAdaptationUnit* acAQU = pcAQLayer->getQPAdaptationUnit();

    Double dMaxQScale = pow(2.0, m_pcEncCfg->getQPAdaptationRange()/6.0);
    Double dAvgAct = pcAQLayer->getAvgActivity();
    Double dCUAct = acAQU[uiAQUPosY * uiAQUStride + uiAQUPosX].getActivity();
    Double dNormAct = (dMaxQScale*dCUAct + dAvgAct) / (dCUAct + dMaxQScale*dAvgAct);
    Double dQpOffset = log(dNormAct) / log(2.0) * 6.0;
    iQpOffset = Int(floor( dQpOffset + 0.49999 ));
  }

  return Clip3(-pcCU->getSlice()->getSPS()->getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQp+iQpOffset );
}

/** encode a CU block recursively
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiDepth
 * \returns Void
 */
Void TEncCu::xEncodeCU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  TComPic* pcPic = pcCU->getPic();

  Bool bBoundary = false;
  UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiRPelX   = uiLPelX + (g_uiMaxCUWidth>>uiDepth)  - 1;
  UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiBPelY   = uiTPelY + (g_uiMaxCUHeight>>uiDepth) - 1;

  TComSlice * pcSlice = pcCU->getPic()->getSlice(pcCU->getPic()->getCurrSliceIdx());
  if( ( uiRPelX < pcSlice->getSPS()->getPicWidthInLumaSamples() ) && ( uiBPelY < pcSlice->getSPS()->getPicHeightInLumaSamples() ) )
  {
    m_pcEntropyCoder->encodeSplitFlag( pcCU, uiAbsPartIdx, uiDepth );
  }
  else
  {
    bBoundary = true;
  }

  if( ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) ) && ( uiDepth < (g_uiMaxCUDepth-g_uiAddCUDepth) ) ) || bBoundary )
  {
    UInt uiQNumParts = ( pcPic->getNumPartitionsInCtu() >> (uiDepth<<1) )>>2;
    if( (g_uiMaxCUWidth>>uiDepth) == pcCU->getSlice()->getPPS()->getMinCuDQPSize() && pcCU->getSlice()->getPPS()->getUseDQP())
    {
      setdQPFlag(true);
    }

    if( (g_uiMaxCUWidth>>uiDepth) == pcCU->getSlice()->getPPS()->getMinCuChromaQpAdjSize() && pcCU->getSlice()->getUseChromaQpAdj())
    {
      setCodeChromaQpAdjFlag(true);
    }

    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++, uiAbsPartIdx+=uiQNumParts )
    {
      uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
      uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
      if( ( uiLPelX < pcSlice->getSPS()->getPicWidthInLumaSamples() ) && ( uiTPelY < pcSlice->getSPS()->getPicHeightInLumaSamples() ) )
      {
        xEncodeCU( pcCU, uiAbsPartIdx, uiDepth+1 );
      }
    }
    return;
  }

  if( (g_uiMaxCUWidth>>uiDepth) >= pcCU->getSlice()->getPPS()->getMinCuDQPSize() && pcCU->getSlice()->getPPS()->getUseDQP())
  {
    setdQPFlag(true);
  }

  if( (g_uiMaxCUWidth>>uiDepth) >= pcCU->getSlice()->getPPS()->getMinCuChromaQpAdjSize() && pcCU->getSlice()->getUseChromaQpAdj())
  {
    setCodeChromaQpAdjFlag(true);
  }

  if (pcCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
  {
    m_pcEntropyCoder->encodeCUTransquantBypassFlag( pcCU, uiAbsPartIdx );
  }

  if( !pcCU->getSlice()->isIntra() )
  {
    m_pcEntropyCoder->encodeSkipFlag( pcCU, uiAbsPartIdx );
  }

  if( pcCU->isSkipped( uiAbsPartIdx ) )
  {
    m_pcEntropyCoder->encodeMergeIndex( pcCU, uiAbsPartIdx );
    finishCU(pcCU,uiAbsPartIdx,uiDepth);
    return;
  }

  m_pcEntropyCoder->encodePredMode( pcCU, uiAbsPartIdx );
  m_pcEntropyCoder->encodePartSize( pcCU, uiAbsPartIdx, uiDepth );

  if (pcCU->isIntra( uiAbsPartIdx ) && pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N )
  {
    m_pcEntropyCoder->encodeIPCMInfo( pcCU, uiAbsPartIdx );

    if(pcCU->getIPCMFlag(uiAbsPartIdx))
    {
      // Encode slice finish
      finishCU(pcCU,uiAbsPartIdx,uiDepth);
      return;
    }
  }

  // prediction Info ( Intra : direction mode, Inter : Mv, reference idx )
  m_pcEntropyCoder->encodePredInfo( pcCU, uiAbsPartIdx );

  // Encode Coefficients
  Bool bCodeDQP = getdQPFlag();
  Bool codeChromaQpAdj = getCodeChromaQpAdjFlag();
  m_pcEntropyCoder->encodeCoeff( pcCU, uiAbsPartIdx, uiDepth, bCodeDQP, codeChromaQpAdj );
  setCodeChromaQpAdjFlag( codeChromaQpAdj );
  setdQPFlag( bCodeDQP );

  // --- write terminating bit ---
  finishCU(pcCU,uiAbsPartIdx,uiDepth);
}

Int xCalcHADs8x8_ISlice(Pel *piOrg, Int iStrideOrg)
{
  Int k, i, j, jj;
  Int diff[64], m1[8][8], m2[8][8], m3[8][8], iSumHad = 0;

  for( k = 0; k < 64; k += 8 )
  {
    diff[k+0] = piOrg[0] ;
    diff[k+1] = piOrg[1] ;
    diff[k+2] = piOrg[2] ;
    diff[k+3] = piOrg[3] ;
    diff[k+4] = piOrg[4] ;
    diff[k+5] = piOrg[5] ;
    diff[k+6] = piOrg[6] ;
    diff[k+7] = piOrg[7] ;

    piOrg += iStrideOrg;
  }

  //horizontal
  for (j=0; j < 8; j++)
  {
    jj = j << 3;
    m2[j][0] = diff[jj  ] + diff[jj+4];
    m2[j][1] = diff[jj+1] + diff[jj+5];
    m2[j][2] = diff[jj+2] + diff[jj+6];
    m2[j][3] = diff[jj+3] + diff[jj+7];
    m2[j][4] = diff[jj  ] - diff[jj+4];
    m2[j][5] = diff[jj+1] - diff[jj+5];
    m2[j][6] = diff[jj+2] - diff[jj+6];
    m2[j][7] = diff[jj+3] - diff[jj+7];

    m1[j][0] = m2[j][0] + m2[j][2];
    m1[j][1] = m2[j][1] + m2[j][3];
    m1[j][2] = m2[j][0] - m2[j][2];
    m1[j][3] = m2[j][1] - m2[j][3];
    m1[j][4] = m2[j][4] + m2[j][6];
    m1[j][5] = m2[j][5] + m2[j][7];
    m1[j][6] = m2[j][4] - m2[j][6];
    m1[j][7] = m2[j][5] - m2[j][7];

    m2[j][0] = m1[j][0] + m1[j][1];
    m2[j][1] = m1[j][0] - m1[j][1];
    m2[j][2] = m1[j][2] + m1[j][3];
    m2[j][3] = m1[j][2] - m1[j][3];
    m2[j][4] = m1[j][4] + m1[j][5];
    m2[j][5] = m1[j][4] - m1[j][5];
    m2[j][6] = m1[j][6] + m1[j][7];
    m2[j][7] = m1[j][6] - m1[j][7];
  }

  //vertical
  for (i=0; i < 8; i++)
  {
    m3[0][i] = m2[0][i] + m2[4][i];
    m3[1][i] = m2[1][i] + m2[5][i];
    m3[2][i] = m2[2][i] + m2[6][i];
    m3[3][i] = m2[3][i] + m2[7][i];
    m3[4][i] = m2[0][i] - m2[4][i];
    m3[5][i] = m2[1][i] - m2[5][i];
    m3[6][i] = m2[2][i] - m2[6][i];
    m3[7][i] = m2[3][i] - m2[7][i];

    m1[0][i] = m3[0][i] + m3[2][i];
    m1[1][i] = m3[1][i] + m3[3][i];
    m1[2][i] = m3[0][i] - m3[2][i];
    m1[3][i] = m3[1][i] - m3[3][i];
    m1[4][i] = m3[4][i] + m3[6][i];
    m1[5][i] = m3[5][i] + m3[7][i];
    m1[6][i] = m3[4][i] - m3[6][i];
    m1[7][i] = m3[5][i] - m3[7][i];

    m2[0][i] = m1[0][i] + m1[1][i];
    m2[1][i] = m1[0][i] - m1[1][i];
    m2[2][i] = m1[2][i] + m1[3][i];
    m2[3][i] = m1[2][i] - m1[3][i];
    m2[4][i] = m1[4][i] + m1[5][i];
    m2[5][i] = m1[4][i] - m1[5][i];
    m2[6][i] = m1[6][i] + m1[7][i];
    m2[7][i] = m1[6][i] - m1[7][i];
  }

  for (i = 0; i < 8; i++)
  {
    for (j = 0; j < 8; j++)
    {
      iSumHad += abs(m2[i][j]);
    }
  }
  iSumHad -= abs(m2[0][0]);
  iSumHad =(iSumHad+2)>>2;
  return(iSumHad);
}

Int  TEncCu::updateCtuDataISlice(TComDataCU* pCtu, Int width, Int height)
{
  Int  xBl, yBl;
  const Int iBlkSize = 8;

  Pel* pOrgInit   = pCtu->getPic()->getPicYuvOrg()->getAddr(COMPONENT_Y, pCtu->getCtuRsAddr(), 0);
  Int  iStrideOrig = pCtu->getPic()->getPicYuvOrg()->getStride(COMPONENT_Y);
  Pel  *pOrg;

  Int iSumHad = 0;
  for ( yBl=0; (yBl+iBlkSize)<=height; yBl+= iBlkSize)
  {
    for ( xBl=0; (xBl+iBlkSize)<=width; xBl+= iBlkSize)
    {
      pOrg = pOrgInit + iStrideOrig*yBl + xBl;
      iSumHad += xCalcHADs8x8_ISlice(pOrg, iStrideOrig);
    }
  }
  return(iSumHad);
}

/** check RD costs for a CU block encoded with merge
 * \param rpcBestCU
 * \param rpcTempCU
 * \returns Void
 */
Void TEncCu::xCheckRDCostMerge2Nx2N( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU DEBUG_STRING_FN_DECLARE(sDebug), Bool *earlyDetectionSkipMode )
{
  assert( rpcTempCU->getSlice()->getSliceType() != I_SLICE );
  TComMvField  cMvFieldNeighbours[2 * MRG_MAX_NUM_CANDS]; // double length for mv of both lists
  UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
  Int numValidMergeCand = 0;
  const Bool bTransquantBypassFlag = rpcTempCU->getCUTransquantBypass(0);

  for( UInt ui = 0; ui < rpcTempCU->getSlice()->getMaxNumMergeCand(); ++ui )
  {
    uhInterDirNeighbours[ui] = 0;
  }
  UChar uhDepth = rpcTempCU->getDepth( 0 );
  rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uhDepth ); // interprets depth relative to CTU level
  rpcTempCU->getInterMergeCandidates( 0, 0, cMvFieldNeighbours,uhInterDirNeighbours, numValidMergeCand );

  Int mergeCandBuffer[MRG_MAX_NUM_CANDS];
  for( UInt ui = 0; ui < numValidMergeCand; ++ui )
  {
    mergeCandBuffer[ui] = 0;
  }

  Bool bestIsSkip = false;

  UInt iteration;
  if ( rpcTempCU->isLosslessCoded(0))
  {
    iteration = 1;
  }
  else
  {
    iteration = 2;
  }
  DEBUG_STRING_NEW(bestStr)

  for( UInt uiNoResidual = 0; uiNoResidual < iteration; ++uiNoResidual )
  {
    for( UInt uiMergeCand = 0; uiMergeCand < numValidMergeCand; ++uiMergeCand )
    {
      if(!(uiNoResidual==1 && mergeCandBuffer[uiMergeCand]==1))
      {
        if( !(bestIsSkip && uiNoResidual == 0) )
        {
          DEBUG_STRING_NEW(tmpStr)
          // set MC parameters
          rpcTempCU->setPredModeSubParts( MODE_INTER, 0, uhDepth ); // interprets depth relative to CTU level
          rpcTempCU->setCUTransquantBypassSubParts( bTransquantBypassFlag, 0, uhDepth );
          rpcTempCU->setChromaQpAdjSubParts( bTransquantBypassFlag ? 0 : m_ChromaQpAdjIdc, 0, uhDepth );
          rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uhDepth ); // interprets depth relative to CTU level
          rpcTempCU->setMergeFlagSubParts( true, 0, 0, uhDepth ); // interprets depth relative to CTU level
          rpcTempCU->setMergeIndexSubParts( uiMergeCand, 0, 0, uhDepth ); // interprets depth relative to CTU level
          rpcTempCU->setInterDirSubParts( uhInterDirNeighbours[uiMergeCand], 0, 0, uhDepth ); // interprets depth relative to CTU level
          rpcTempCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( cMvFieldNeighbours[0 + 2*uiMergeCand], SIZE_2Nx2N, 0, 0 ); // interprets depth relative to rpcTempCU level
          rpcTempCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( cMvFieldNeighbours[1 + 2*uiMergeCand], SIZE_2Nx2N, 0, 0 ); // interprets depth relative to rpcTempCU level

          // do MC
          m_pcPredSearch->motionCompensation ( rpcTempCU, m_ppcPredYuvTemp[uhDepth] );
          // estimate residual and encode everything
          m_pcPredSearch->encodeResAndCalcRdInterCU( rpcTempCU,
                                                     m_ppcOrigYuv    [uhDepth],
                                                     m_ppcPredYuvTemp[uhDepth],
                                                     m_ppcResiYuvTemp[uhDepth],
                                                     m_ppcResiYuvBest[uhDepth],
                                                     m_ppcRecoYuvTemp[uhDepth],
                                                     (uiNoResidual != 0) DEBUG_STRING_PASS_INTO(tmpStr) );

#ifdef DEBUG_STRING
          DebugInterPredResiReco(tmpStr, *(m_ppcPredYuvTemp[uhDepth]), *(m_ppcResiYuvBest[uhDepth]), *(m_ppcRecoYuvTemp[uhDepth]), DebugStringGetPredModeMask(rpcTempCU->getPredictionMode(0)));
#endif

          if ((uiNoResidual == 0) && (rpcTempCU->getQtRootCbf(0) == 0))
          {
            // If no residual when allowing for one, then set mark to not try case where residual is forced to 0
            mergeCandBuffer[uiMergeCand] = 1;
          }

          rpcTempCU->setSkipFlagSubParts( rpcTempCU->getQtRootCbf(0) == 0, 0, uhDepth );
          Int orgQP = rpcTempCU->getQP( 0 );
          xCheckDQP( rpcTempCU );
          xCheckBestMode(rpcBestCU, rpcTempCU, uhDepth DEBUG_STRING_PASS_INTO(bestStr) DEBUG_STRING_PASS_INTO(tmpStr));

          rpcTempCU->initEstData( uhDepth, orgQP, bTransquantBypassFlag );

          if( m_pcEncCfg->getUseFastDecisionForMerge() && !bestIsSkip )
          {
            bestIsSkip = rpcBestCU->getQtRootCbf(0) == 0;
          }
        }
      }
    }

    if(uiNoResidual == 0 && m_pcEncCfg->getUseEarlySkipDetection())
    {
      if(rpcBestCU->getQtRootCbf( 0 ) == 0)
      {
        if( rpcBestCU->getMergeFlag( 0 ))
        {
          *earlyDetectionSkipMode = true;
        }
        else if(m_pcEncCfg->getFastSearch() != SELECTIVE)
        {
          Int absoulte_MV=0;
          for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
          {
            if ( rpcBestCU->getSlice()->getNumRefIdx( RefPicList( uiRefListIdx ) ) > 0 )
            {
              TComCUMvField* pcCUMvField = rpcBestCU->getCUMvField(RefPicList( uiRefListIdx ));
              Int iHor = pcCUMvField->getMvd( 0 ).getAbsHor();
              Int iVer = pcCUMvField->getMvd( 0 ).getAbsVer();
              absoulte_MV+=iHor+iVer;
            }
          }

          if(absoulte_MV == 0)
          {
            *earlyDetectionSkipMode = true;
          }
        }
      }
    }
  }
  DEBUG_STRING_APPEND(sDebug, bestStr)
}


#if AMP_MRG
Void TEncCu::xCheckRDCostInter( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize DEBUG_STRING_FN_DECLARE(sDebug), Bool bUseMRG)
#else
Void TEncCu::xCheckRDCostInter( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize )
#endif
{
  DEBUG_STRING_NEW(sTest)

  UChar uhDepth = rpcTempCU->getDepth( 0 );

  rpcTempCU->setDepthSubParts( uhDepth, 0 );

  rpcTempCU->setSkipFlagSubParts( false, 0, uhDepth );

  rpcTempCU->setPartSizeSubParts  ( ePartSize,  0, uhDepth );
  rpcTempCU->setPredModeSubParts  ( MODE_INTER, 0, uhDepth );
  rpcTempCU->setChromaQpAdjSubParts( rpcTempCU->getCUTransquantBypass(0) ? 0 : m_ChromaQpAdjIdc, 0, uhDepth );

#if AMP_MRG
  rpcTempCU->setMergeAMP (true);
  m_pcPredSearch->predInterSearch ( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcRecoYuvTemp[uhDepth] DEBUG_STRING_PASS_INTO(sTest), false, bUseMRG );
#else
  m_pcPredSearch->predInterSearch ( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcRecoYuvTemp[uhDepth] );
#endif

#if AMP_MRG
  if ( !rpcTempCU->getMergeAMP() )
  {
    return;
  }
#endif

  m_pcPredSearch->encodeResAndCalcRdInterCU( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcResiYuvBest[uhDepth], m_ppcRecoYuvTemp[uhDepth], false DEBUG_STRING_PASS_INTO(sTest) );
  rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );

#ifdef DEBUG_STRING
  DebugInterPredResiReco(sTest, *(m_ppcPredYuvTemp[uhDepth]), *(m_ppcResiYuvBest[uhDepth]), *(m_ppcRecoYuvTemp[uhDepth]), DebugStringGetPredModeMask(rpcTempCU->getPredictionMode(0)));
#endif

  xCheckDQP( rpcTempCU );
  xCheckBestMode(rpcBestCU, rpcTempCU, uhDepth DEBUG_STRING_PASS_INTO(sDebug) DEBUG_STRING_PASS_INTO(sTest));
}

Void TEncCu::xCheckRDCostIntra( TComDataCU *&rpcBestCU,
                                TComDataCU *&rpcTempCU,
                                Double      &cost,
                                PartSize     eSize
                                DEBUG_STRING_FN_DECLARE(sDebug) )
{
  DEBUG_STRING_NEW(sTest)

  UInt uiDepth = rpcTempCU->getDepth( 0 );

  rpcTempCU->setSkipFlagSubParts( false, 0, uiDepth );

  rpcTempCU->setPartSizeSubParts( eSize, 0, uiDepth );
  rpcTempCU->setPredModeSubParts( MODE_INTRA, 0, uiDepth );
  rpcTempCU->setChromaQpAdjSubParts( rpcTempCU->getCUTransquantBypass(0) ? 0 : m_ChromaQpAdjIdc, 0, uiDepth );

  Bool bSeparateLumaChroma = true; // choose estimation mode

  Distortion uiPreCalcDistC = 0;
  if (rpcBestCU->getPic()->getChromaFormat()==CHROMA_400)
  {
    bSeparateLumaChroma=true;
  }

  Pel resiLuma[NUMBER_OF_STORED_RESIDUAL_TYPES][MAX_CU_SIZE * MAX_CU_SIZE];

  if( !bSeparateLumaChroma )
  {
    // after this function, the direction will be PLANAR, DC, HOR or VER
    // however, if Luma ends up being one of those, the chroma dir must be later changed to DM_CHROMA.
    m_pcPredSearch->preestChromaPredMode( rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth] );
  }
  m_pcPredSearch->estIntraPredQT( rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth], resiLuma, uiPreCalcDistC, bSeparateLumaChroma DEBUG_STRING_PASS_INTO(sTest) );

  m_ppcRecoYuvTemp[uiDepth]->copyToPicComponent(COMPONENT_Y, rpcTempCU->getPic()->getPicYuvRec(), rpcTempCU->getCtuRsAddr(), rpcTempCU->getZorderIdxInCtu() );

  if (rpcBestCU->getPic()->getChromaFormat()!=CHROMA_400)
  {
    m_pcPredSearch->estIntraPredChromaQT( rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth], resiLuma, uiPreCalcDistC DEBUG_STRING_PASS_INTO(sTest) );
  }

  m_pcEntropyCoder->resetBits();

  if ( rpcTempCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
  {
    m_pcEntropyCoder->encodeCUTransquantBypassFlag( rpcTempCU, 0,          true );
  }

  m_pcEntropyCoder->encodeSkipFlag ( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodePredMode( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodePartSize( rpcTempCU, 0, uiDepth, true );
  m_pcEntropyCoder->encodePredInfo( rpcTempCU, 0 );
  m_pcEntropyCoder->encodeIPCMInfo(rpcTempCU, 0, true );

  // Encode Coefficients
  Bool bCodeDQP = getdQPFlag();
  Bool codeChromaQpAdjFlag = getCodeChromaQpAdjFlag();
  m_pcEntropyCoder->encodeCoeff( rpcTempCU, 0, uiDepth, bCodeDQP, codeChromaQpAdjFlag );
  setCodeChromaQpAdjFlag( codeChromaQpAdjFlag );
  setdQPFlag( bCodeDQP );

  m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);

  rpcTempCU->getTotalBits() = m_pcEntropyCoder->getNumberOfWrittenBits();
  rpcTempCU->getTotalBins() = ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
  rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );

  xCheckDQP( rpcTempCU );

  cost = rpcTempCU->getTotalCost();

  xCheckBestMode(rpcBestCU, rpcTempCU, uiDepth DEBUG_STRING_PASS_INTO(sDebug) DEBUG_STRING_PASS_INTO(sTest));
}


/** Check R-D costs for a CU with PCM mode.
 * \param rpcBestCU pointer to best mode CU data structure
 * \param rpcTempCU pointer to bOnlyDepth0ng mode CU data structure
 * \returns Void
 *
 * \note Current PCM implementation encodes sample values in a lossless way. The distortion of PCM mode CUs are zero. PCM mode is selected if the best mode yields bits greater than that of PCM mode.
 */
Void TEncCu::xCheckIntraPCM( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU )
{
  UInt uiDepth = rpcTempCU->getDepth( 0 );

  rpcTempCU->setSkipFlagSubParts( false, 0, uiDepth );

  rpcTempCU->setIPCMFlag(0, true);
  rpcTempCU->setIPCMFlagSubParts (true, 0, rpcTempCU->getDepth(0));
  rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uiDepth );
  rpcTempCU->setPredModeSubParts( MODE_INTRA, 0, uiDepth );
  rpcTempCU->setTrIdxSubParts ( 0, 0, uiDepth );
  rpcTempCU->setChromaQpAdjSubParts( rpcTempCU->getCUTransquantBypass(0) ? 0 : m_ChromaQpAdjIdc, 0, uiDepth );

  m_pcPredSearch->IPCMSearch( rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth]);

  m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

  m_pcEntropyCoder->resetBits();

  if ( rpcTempCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
  {
    m_pcEntropyCoder->encodeCUTransquantBypassFlag( rpcTempCU, 0,          true );
  }

  m_pcEntropyCoder->encodeSkipFlag ( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodePredMode ( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodePartSize ( rpcTempCU, 0, uiDepth, true );
  m_pcEntropyCoder->encodeIPCMInfo ( rpcTempCU, 0, true );

  m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);

  rpcTempCU->getTotalBits() = m_pcEntropyCoder->getNumberOfWrittenBits();
  rpcTempCU->getTotalBins() = ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
  rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );

  xCheckDQP( rpcTempCU );
  DEBUG_STRING_NEW(a)
  DEBUG_STRING_NEW(b)
  xCheckBestMode(rpcBestCU, rpcTempCU, uiDepth DEBUG_STRING_PASS_INTO(a) DEBUG_STRING_PASS_INTO(b));
}

/** check whether current try is the best with identifying the depth of current try
 * \param rpcBestCU
 * \param rpcTempCU
 * \returns Void
 */
Void TEncCu::xCheckBestMode( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth DEBUG_STRING_FN_DECLARE(sParent) DEBUG_STRING_FN_DECLARE(sTest) DEBUG_STRING_PASS_INTO(Bool bAddSizeInfo) )
{
  if( rpcTempCU->getTotalCost() < rpcBestCU->getTotalCost() )
  {
    TComYuv* pcYuv;
    // Change Information data
    TComDataCU* pcCU = rpcBestCU;
    rpcBestCU = rpcTempCU;
    rpcTempCU = pcCU;

    // Change Prediction data
    pcYuv = m_ppcPredYuvBest[uiDepth];
    m_ppcPredYuvBest[uiDepth] = m_ppcPredYuvTemp[uiDepth];
    m_ppcPredYuvTemp[uiDepth] = pcYuv;

    // Change Reconstruction data
    pcYuv = m_ppcRecoYuvBest[uiDepth];
    m_ppcRecoYuvBest[uiDepth] = m_ppcRecoYuvTemp[uiDepth];
    m_ppcRecoYuvTemp[uiDepth] = pcYuv;

    pcYuv = NULL;
    pcCU  = NULL;

    // store temp best CI for next CU coding
    m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]->store(m_pppcRDSbacCoder[uiDepth][CI_NEXT_BEST]);


#ifdef DEBUG_STRING
    DEBUG_STRING_SWAP(sParent, sTest)
    const PredMode predMode=rpcBestCU->getPredictionMode(0);
    if ((DebugOptionList::DebugString_Structure.getInt()&DebugStringGetPredModeMask(predMode)) && bAddSizeInfo)
    {
      std::stringstream ss(stringstream::out);
      ss <<"###: " << (predMode==MODE_INTRA?"Intra   ":"Inter   ") << partSizeToString[rpcBestCU->getPartitionSize(0)] << " CU at " << rpcBestCU->getCUPelX() << ", " << rpcBestCU->getCUPelY() << " width=" << UInt(rpcBestCU->getWidth(0)) << std::endl;
      sParent+=ss.str();
    }
#endif
  }
}

Void TEncCu::xCheckDQP( TComDataCU* pcCU )
{
  UInt uiDepth = pcCU->getDepth( 0 );

  if( pcCU->getSlice()->getPPS()->getUseDQP() && (g_uiMaxCUWidth>>uiDepth) >= pcCU->getSlice()->getPPS()->getMinCuDQPSize() )
  {
    if ( pcCU->getQtRootCbf( 0) )
    {
#if !RDO_WITHOUT_DQP_BITS
      m_pcEntropyCoder->resetBits();
      m_pcEntropyCoder->encodeQP( pcCU, 0, false );
      pcCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // dQP bits
      pcCU->getTotalBins() += ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
      pcCU->getTotalCost() = m_pcRdCost->calcRdCost( pcCU->getTotalBits(), pcCU->getTotalDistortion() );
#endif
    }
    else
    {
      pcCU->setQPSubParts( pcCU->getRefQP( 0 ), 0, uiDepth ); // set QP to default QP
    }
  }
}

Void TEncCu::xCopyAMVPInfo (AMVPInfo* pSrc, AMVPInfo* pDst)
{
  pDst->iN = pSrc->iN;
  for (Int i = 0; i < pSrc->iN; i++)
  {
    pDst->m_acMvCand[i] = pSrc->m_acMvCand[i];
  }
}
Void TEncCu::xCopyYuv2Pic(TComPic* rpcPic, UInt uiCUAddr, UInt uiAbsPartIdx, UInt uiDepth, UInt uiSrcDepth, TComDataCU* pcCU, UInt uiLPelX, UInt uiTPelY )
{
  UInt uiAbsPartIdxInRaster = g_auiZscanToRaster[uiAbsPartIdx];
  UInt uiSrcBlkWidth = rpcPic->getNumPartInCtuWidth() >> (uiSrcDepth);
  UInt uiBlkWidth    = rpcPic->getNumPartInCtuWidth() >> (uiDepth);
  UInt uiPartIdxX = ( ( uiAbsPartIdxInRaster % rpcPic->getNumPartInCtuWidth() ) % uiSrcBlkWidth) / uiBlkWidth;
  UInt uiPartIdxY = ( ( uiAbsPartIdxInRaster / rpcPic->getNumPartInCtuWidth() ) % uiSrcBlkWidth) / uiBlkWidth;
  UInt uiPartIdx = uiPartIdxY * ( uiSrcBlkWidth / uiBlkWidth ) + uiPartIdxX;
  m_ppcRecoYuvBest[uiSrcDepth]->copyToPicYuv( rpcPic->getPicYuvRec (), uiCUAddr, uiAbsPartIdx, uiDepth - uiSrcDepth, uiPartIdx);

  m_ppcPredYuvBest[uiSrcDepth]->copyToPicYuv( rpcPic->getPicYuvPred (), uiCUAddr, uiAbsPartIdx, uiDepth - uiSrcDepth, uiPartIdx);
}

Void TEncCu::xCopyYuv2Tmp( UInt uiPartUnitIdx, UInt uiNextDepth )
{
  UInt uiCurrDepth = uiNextDepth - 1;
  m_ppcRecoYuvBest[uiNextDepth]->copyToPartYuv( m_ppcRecoYuvTemp[uiCurrDepth], uiPartUnitIdx );
  m_ppcPredYuvBest[uiNextDepth]->copyToPartYuv( m_ppcPredYuvBest[uiCurrDepth], uiPartUnitIdx);
}

/** Function for filling the PCM buffer of a CU using its original sample array
 * \param pcCU pointer to current CU
 * \param pcOrgYuv pointer to original sample array
 * \returns Void
 */
Void TEncCu::xFillPCMBuffer     ( TComDataCU* pCU, TComYuv* pOrgYuv )
{
  const ChromaFormat format = pCU->getPic()->getChromaFormat();
  const UInt numberValidComponents = getNumberValidComponents(format);
  for (UInt componentIndex = 0; componentIndex < numberValidComponents; componentIndex++)
  {
    const ComponentID component = ComponentID(componentIndex);

    const UInt width  = pCU->getWidth(0)  >> getComponentScaleX(component, format);
    const UInt height = pCU->getHeight(0) >> getComponentScaleY(component, format);

    Pel *source      = pOrgYuv->getAddr(component, 0, width);
    Pel *destination = pCU->getPCMSample(component);

    const UInt sourceStride = pOrgYuv->getStride(component);

    for (Int line = 0; line < height; line++)
    {
      for (Int column = 0; column < width; column++)
      {
        destination[column] = source[column];
      }

      source      += sourceStride;
      destination += width;
    }
  }
}

#if ADAPTIVE_QP_SELECTION
/** Collect ARL statistics from one block
  */
Int TEncCu::xTuCollectARLStats(TCoeff* rpcCoeff, TCoeff* rpcArlCoeff, Int NumCoeffInCU, Double* cSum, UInt* numSamples )
{
  for( Int n = 0; n < NumCoeffInCU; n++ )
  {
    TCoeff u = abs( rpcCoeff[ n ] );
    TCoeff absc = rpcArlCoeff[ n ];

    if( u != 0 )
    {
      if( u < LEVEL_RANGE )
      {
        cSum[ u ] += ( Double )absc;
        numSamples[ u ]++;
      }
      else
      {
        cSum[ LEVEL_RANGE ] += ( Double )absc - ( Double )( u << ARL_C_PRECISION );
        numSamples[ LEVEL_RANGE ]++;
      }
    }
  }

  return 0;
}

/** Collect ARL statistics from one CTU
 * \param pcCU
 */
Void TEncCu::xCtuCollectARLStats(TComDataCU* pCtu )
{
  Double cSum[ LEVEL_RANGE + 1 ];     //: the sum of DCT coefficients corresponding to datatype and quantization output
  UInt numSamples[ LEVEL_RANGE + 1 ]; //: the number of coefficients corresponding to datatype and quantization output

  TCoeff* pCoeffY = pCtu->getCoeff(COMPONENT_Y);
  TCoeff* pArlCoeffY = pCtu->getArlCoeff(COMPONENT_Y);

  UInt uiMinCUWidth = g_uiMaxCUWidth >> g_uiMaxCUDepth;
  UInt uiMinNumCoeffInCU = 1 << uiMinCUWidth;

  memset( cSum, 0, sizeof( Double )*(LEVEL_RANGE+1) );
  memset( numSamples, 0, sizeof( UInt )*(LEVEL_RANGE+1) );

  // Collect stats to cSum[][] and numSamples[][]
  for(Int i = 0; i < pCtu->getTotalNumPart(); i ++ )
  {
    UInt uiTrIdx = pCtu->getTransformIdx(i);

    if(pCtu->isInter(i) && pCtu->getCbf( i, COMPONENT_Y, uiTrIdx ) )
    {
      xTuCollectARLStats(pCoeffY, pArlCoeffY, uiMinNumCoeffInCU, cSum, numSamples);
    }//Note that only InterY is processed. QP rounding is based on InterY data only.

    pCoeffY  += uiMinNumCoeffInCU;
    pArlCoeffY  += uiMinNumCoeffInCU;
  }

  for(Int u=1; u<LEVEL_RANGE;u++)
  {
    m_pcTrQuant->getSliceSumC()[u] += cSum[ u ] ;
    m_pcTrQuant->getSliceNSamples()[u] += numSamples[ u ] ;
  }
  m_pcTrQuant->getSliceSumC()[LEVEL_RANGE] += cSum[ LEVEL_RANGE ] ;
  m_pcTrQuant->getSliceNSamples()[LEVEL_RANGE] += numSamples[ LEVEL_RANGE ] ;
}
#endif


/** Determines which depth are adopted by the Left neighbour CTU
*\param   pcCu
*\param   bAdoptedCUDepths
*\param   R
*\returns Void
*/
Void TEncCu::getAdoptedDepthsLeft(TComDataCU* pcCU, Bool* bAdoptedCUDepths, UInt R)
{
  // initialize
  for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
  {
    bAdoptedCUDepths[ui] = false;
  }

  TComDataCU* leftCU = pcCU->getCtuLeft();
  if (leftCU == NULL)
  {
    return;
  }

  if (R == 8)
  {
    bAdoptedCUDepths[leftCU->getDepth(84)]  = true;
    bAdoptedCUDepths[leftCU->getDepth(92)]  = true;
    bAdoptedCUDepths[leftCU->getDepth(116)] = true;
    bAdoptedCUDepths[leftCU->getDepth(124)] = true;
    bAdoptedCUDepths[leftCU->getDepth(212)] = true;
    bAdoptedCUDepths[leftCU->getDepth(220)] = true;
    bAdoptedCUDepths[leftCU->getDepth(244)] = true;
    bAdoptedCUDepths[leftCU->getDepth(252)] = true;
  }
  else if (R == 16)
  {
    for (UInt uj = 80; uj < 144; uj = uj + 32)
    {
      for (UInt ui = 0; ui < 16; ui = ui + 4)
      {
        bAdoptedCUDepths[leftCU->getDepth(uj + ui)] = true;
      }
    }
    for (UInt uj = 208; uj < 272; uj = uj + 32)
    {
      for (UInt ui = 0; ui < 16; ui = ui + 4)
      {
        bAdoptedCUDepths[leftCU->getDepth(uj + ui)] = true;
      }
    }
  }
  else if (R == 32)
  {
    for (UInt uj = 64; uj < 320; uj = uj + 128)
    {
      for (UInt ui = 0; ui < 64; ui = ui + 4)
      {
        bAdoptedCUDepths[leftCU->getDepth(uj + ui)] = true;
      }
    }
  }
  else  // R = 64
  {
    for (UInt ui = 0; ui < 256; ui = ui + 4)
    {
      bAdoptedCUDepths[leftCU->getDepth(ui)] = true;
    }
  }
}

/** Determines which depth are adopted by the Above neighbour CTU
*\param   pcCu
*\param   bAdoptedCUDepths
*\param   R
*\returns Void
*/
Void TEncCu::getAdoptedDepthsAbove(TComDataCU* pcCU, Bool* bAdoptedCUDepths, UInt R)
{
  // initialize
  for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
  {
    bAdoptedCUDepths[ui] = false;
  }

  TComDataCU* aboveCU = pcCU->getCtuAbove();
  if (aboveCU == NULL)
  {
    return;
  }

  if (R == 8)
  {
    bAdoptedCUDepths[aboveCU->getDepth(168)] = true;
    bAdoptedCUDepths[aboveCU->getDepth(172)] = true;
    bAdoptedCUDepths[aboveCU->getDepth(184)] = true;
    bAdoptedCUDepths[aboveCU->getDepth(188)] = true;
    bAdoptedCUDepths[aboveCU->getDepth(232)] = true;
    bAdoptedCUDepths[aboveCU->getDepth(236)] = true;
    bAdoptedCUDepths[aboveCU->getDepth(248)] = true;
    bAdoptedCUDepths[aboveCU->getDepth(252)] = true;
  }
  else if (R == 16)
  {
    for (UInt uj = 160; uj < 192; uj = uj + 16)
    {
      for (UInt ui = 0; ui < 16; ui = ui + 4)
      {
        bAdoptedCUDepths[aboveCU->getDepth(uj + ui)] = true;
      }
    }
    for (UInt uj = 224; uj < 256; uj = uj + 16)
    {
      for (UInt ui = 0; ui < 16; ui = ui + 4)
      {
        bAdoptedCUDepths[aboveCU->getDepth(uj + ui)] = true;
      }
    }
  }
  else if (R == 32)
  {
    for (UInt ui = 128; ui < 256; ui = ui + 4)
    {
      bAdoptedCUDepths[aboveCU->getDepth(ui)] = true;
    }
  }
  else  // R = 64
  {
    for (UInt ui = 0; ui < 256; ui = ui + 4)
    {
      bAdoptedCUDepths[aboveCU->getDepth(ui)] = true;
    }
  }
}

/** Determines which depth are adopted by the Above Left neighbour CTU
*\param   pcCu
*\param   bAdoptedCUDepths
*\param   R
*\returns Void
*/
Void TEncCu::getAdoptedDepthsAboveLeft(TComDataCU* pcCU, Bool* bAdoptedCUDepths, UInt R)
{
  // initialize
  for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
  {
    bAdoptedCUDepths[ui] = false;
  }

  TComDataCU* aboveLeftCU = pcCU->getCtuAboveLeft();
  if (aboveLeftCU == NULL)
  {
    return;
  }

  if (R == 8)
  {
    bAdoptedCUDepths[aboveLeftCU->getDepth(252)]  = true;
  }
  else if (R == 16)
  {
    for (UInt ui = 240; ui < 256; ui = ui + 4)
    {
      bAdoptedCUDepths[aboveLeftCU->getDepth(ui)] = true;
    }
  }
  else if (R == 32)
  {
    for (UInt ui = 192; ui < 256; ui = ui + 4)
    {
      bAdoptedCUDepths[aboveLeftCU->getDepth(ui)] = true;
    }
  }
  else  // R = 64
  {
    for (UInt ui = 0; ui < 256; ui = ui + 4)
    {
      bAdoptedCUDepths[aboveLeftCU->getDepth(ui)] = true;
    }
  }

  memcpy(m_bAdoptedByC, bAdoptedCUDepths, sizeof(Bool)*(m_uhTotalDepth - 1));
}

/** Determines which depth are adopted by the Above Right neighbour CTU
*\param   pcCu
*\param   bAdoptedCUDepths
*\param   R
*\returns Void
*/
Void TEncCu::getAdoptedDepthsAboveRight(TComDataCU* pcCU, Bool* bAdoptedCUDepths, UInt R)
{
  // initialize
  for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
  {
    bAdoptedCUDepths[ui] = false;
  }

  TComDataCU* aboveRightCU = pcCU->getCtuAboveRight();
  if (aboveRightCU == NULL)
  {
    return;
  }

  if (R == 8)
  {
    bAdoptedCUDepths[aboveRightCU->getDepth(168)]  = true;
  }
  else if (R == 16)
  {
    for (UInt ui = 160; ui < 176; ui = ui + 4)
    {
      bAdoptedCUDepths[aboveRightCU->getDepth(ui)] = true;
    }
  }
  else if (R == 32)
  {
    for (UInt ui = 128; ui < 192; ui = ui + 4)
    {
      bAdoptedCUDepths[aboveRightCU->getDepth(ui)] = true;
    }
  }
  else  // R = 64
  {
    for (UInt ui = 0; ui < 256; ui = ui + 4)
    {
      bAdoptedCUDepths[aboveRightCU->getDepth(ui)] = true;
    }
  }
}

/** Determines which depth are adopted by the Right neighbour CTU
*\param   pcCu
*\param   bAdoptedCUDepths
*\param   R
*\returns Void
*/
Void TEncCu::getAdoptedDepthsRight(TComDataCU* pcCU, Bool* bAdoptedCUDepths, UInt R)
{
  // initialize
  for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
  {
    bAdoptedCUDepths[ui] = false;
  }

  TComDataCU* rightCU = pcCU->getCtuRight();
  if (rightCU == NULL)
  {
    return;
  }

  if (R == 8)
  {
    bAdoptedCUDepths[rightCU->getDepth(0)]   = true;
    bAdoptedCUDepths[rightCU->getDepth(8)]   = true;
    bAdoptedCUDepths[rightCU->getDepth(32)]  = true;
    bAdoptedCUDepths[rightCU->getDepth(40)]  = true;
    bAdoptedCUDepths[rightCU->getDepth(128)] = true;
    bAdoptedCUDepths[rightCU->getDepth(136)] = true;
    bAdoptedCUDepths[rightCU->getDepth(160)] = true;
    bAdoptedCUDepths[rightCU->getDepth(168)] = true;
  }
  else if (R == 16)
  {
    for (UInt uj = 0; uj < 48; uj = uj + 32)
    {
      for (UInt ui = 0; ui < 16; ui = ui + 4)
      {
        bAdoptedCUDepths[rightCU->getDepth(uj + ui)] = true;
      }
    }
    for (UInt uj = 128; uj < 176; uj = uj + 32)
    {
      for (UInt ui = 0; ui < 16; ui = ui + 4)
      {
        bAdoptedCUDepths[rightCU->getDepth(uj + ui)] = true;
      }
    }
  }
  else if (R == 32)
  {
    for (UInt uj = 0; uj < 192; uj = uj + 128)
    {
      for (UInt ui = 0; ui < 64; ui = ui + 4)
      {
        bAdoptedCUDepths[rightCU->getDepth(ui)] = true;
      }
    }
  }
  else  // R = 64
  {
    for (UInt ui = 0; ui < 256; ui = ui + 4)
    {
      bAdoptedCUDepths[rightCU->getDepth(ui)] = true;
    }
  }
}

/** Determines which depth are adopted by the Bottom neighbour CTU
*\param   pcCu
*\param   bAdoptedCUDepths
*\param   R
*\returns Void
*/
Void TEncCu::getAdoptedDepthsBottom(TComDataCU* pcCU, Bool* bAdoptedCUDepths, UInt R)
{
  // initialize
  for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
  {
    bAdoptedCUDepths[ui] = false;
  }

  TComDataCU* bottomCU = pcCU->getCtuBottom();
  if (bottomCU == NULL)
  {
    return;
  }

  if (R == 8)
  {
    bAdoptedCUDepths[bottomCU->getDepth(0)]  = true;
    bAdoptedCUDepths[bottomCU->getDepth(4)]  = true;
    bAdoptedCUDepths[bottomCU->getDepth(16)] = true;
    bAdoptedCUDepths[bottomCU->getDepth(20)] = true;
    bAdoptedCUDepths[bottomCU->getDepth(64)] = true;
    bAdoptedCUDepths[bottomCU->getDepth(68)] = true;
    bAdoptedCUDepths[bottomCU->getDepth(80)] = true;
    bAdoptedCUDepths[bottomCU->getDepth(84)] = true;
  }
  else if (R == 16)
  {
    for (UInt uj = 0; uj < 32; uj = uj + 16)
    {
      for (UInt ui = 0; ui < 16; ui = ui + 4)
      {
        bAdoptedCUDepths[bottomCU->getDepth(uj + ui)] = true;
      }
    }
    for (UInt uj = 64; uj < 96; uj = uj + 16)
    {
      for (UInt ui = 0; ui < 16; ui = ui + 4)
      {
        bAdoptedCUDepths[bottomCU->getDepth(uj + ui)] = true;
      }
    }
  }
  else if (R == 32)
  {
    for (UInt uj = 0; uj < 128; uj = uj + 64)
    {
      for (UInt ui = 0; ui < 64; ui = ui + 4)
      {
        bAdoptedCUDepths[bottomCU->getDepth(ui)] = true;
      }
    }
  }
  else  // R = 64
  {
    for (UInt ui = 0; ui < 256; ui = ui + 4)
    {
      bAdoptedCUDepths[bottomCU->getDepth(ui)] = true;
    }
  }
}

/** Determines which depth are adopted by the previous Colocated CTU
*\param   pcCu
*\param   bAdoptedCUDepths
*\param   R
*\returns Void
*/
Void TEncCu::getAdoptedDepthsColocated(TComDataCU* pcCU, Bool* bAdoptedCUDepths, UInt R)
{
  // initialize
  for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
  {
    bAdoptedCUDepths[ui] = false;
  }

  TComDataCU* colocatedCU = pcCU->getCUColocated(REF_PIC_LIST_0);
  if (colocatedCU == NULL)
  {
    return;
  }

  for (UInt ui = 0; ui < 256; ui = ui + 4)
  {
    bAdoptedCUDepths[colocatedCU->getDepth(ui)] = true;
  }

  memcpy(m_bAdoptedByColocated, bAdoptedCUDepths, sizeof(Bool)*(m_uhTotalDepth - 1));
}

/** initiliaze the array that holds the depths adopted by group alpha
*\returns Void
*/
Void TEncCu::initGroupAlpha()
{
  for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
  {
    m_uiAlphaDepths[ui] = 0;
    m_bAdoptedByC[ui] = false;
  }

  m_uiSizeAlpha = 0;
}

/** updates the depths array of group alpha with data from the last CTU
*\param   bAdoptedCUDepths
*\returns Void
*/
Void TEncCu::updateGroupAlpha(Bool* bAdoptedCUDepths)
{
  bool newAlphaCU = false;

  for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
  {
    if (bAdoptedCUDepths[ui] == true)
    {
      m_uiAlphaDepths[ui]++; // increase each adopted depth only once for the current CTU
      newAlphaCU = true;
    }
  }

  if (newAlphaCU)
  {
    m_uiSizeAlpha++;
  }
}

/** build the array of depths adopted by the basic group alpha with respect to the supplied CTU
*\param   pcCU
*\returns Void
*/
Void TEncCu::buildGroupAlpha(TComDataCU* pcCU)
{
  Bool* bAdoptedCUDepths = new Bool[m_uhTotalDepth - 1];
  UInt R = m_pcEncCfg->getR();
  initGroupAlpha();

  getAdoptedDepthsLeft(pcCU, bAdoptedCUDepths, R);
  updateGroupAlpha(bAdoptedCUDepths);
  getAdoptedDepthsAbove(pcCU, bAdoptedCUDepths, R);
  updateGroupAlpha(bAdoptedCUDepths);
  getAdoptedDepthsAboveLeft(pcCU, bAdoptedCUDepths, R);
  updateGroupAlpha(bAdoptedCUDepths);
  getAdoptedDepthsColocated(pcCU, bAdoptedCUDepths, R);
  updateGroupAlpha(bAdoptedCUDepths);

  delete bAdoptedCUDepths;
}

/** initiliaze the array that holds the depths adopted by group beta
*\returns Void
*/
Void TEncCu::initGroupBeta()
{
  for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
  {
    m_uiBetaDepths[ui] = 0;
  }
}

/** updates the depths array of group beta with data from the last CTU
*\param   bAdoptedCUDepths
*\returns Void
*/
Void TEncCu::updateGroupBeta(Bool* bAdoptedCUDepths)
{
  for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
  {
    if (bAdoptedCUDepths[ui] == true)
    {
      m_uiBetaDepths[ui]++; // increase each adopted depth only once for the current CTU
    }
  }
}

/** build the array of depths adopted by the extended group beta with respect to the supplied CTU
*\param   pcCU
*\returns Void
*/
Void TEncCu::buildGroupBeta(TComDataCU* pcCU)
{
  TComDataCU* pcColocatedCU = pcCU->getCUColocated(REF_PIC_LIST_0);
  Bool* bAdoptedCUDepths = new Bool[m_uhTotalDepth - 1];
  UInt R = m_pcEncCfg->getR();
  initGroupBeta();

  getAdoptedDepthsAboveRight(pcCU, bAdoptedCUDepths, R);
  updateGroupBeta(bAdoptedCUDepths);
  getAdoptedDepthsLeft(pcColocatedCU, bAdoptedCUDepths, R);
  updateGroupBeta(bAdoptedCUDepths);
  getAdoptedDepthsAbove(pcColocatedCU, bAdoptedCUDepths, R);
  updateGroupBeta(bAdoptedCUDepths);
  getAdoptedDepthsRight(pcColocatedCU, bAdoptedCUDepths, R);
  updateGroupBeta(bAdoptedCUDepths);
  getAdoptedDepthsBottom(pcColocatedCU, bAdoptedCUDepths, R);
  updateGroupBeta(bAdoptedCUDepths);

  delete bAdoptedCUDepths;
}

/** determine if the depths adopted by group beta are also adopted by group alpha
*\returns Bool
*/
Bool TEncCu::isBetaIncludedInAlpha()
{
  for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
  {
    if (m_uiAlphaDepths[ui] == 0 && m_uiBetaDepths[ui] > 0)
    {
      return false;
    }
  }

  return true;
}

/** calculate and return the similarity level as explained in the article by R. Fan
*\returns UInt
*/
UInt TEncCu::getSimLevel()
{
  UInt uiCount = 0;

  for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
  {
    if (m_uiAlphaDepths[ui] > 0)
    {
      uiCount++;
    }
  }

  return uiCount;
}

/** in medium high similarity, indicates whether CTU C is the only CTU to adopt a certain depth level in groups alpha and beta
/** building groups alpha and beta is a must before running this function
*\returns Bool
*/
Bool TEncCu::isOnlyAdoptedbyC()
{
  for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
  {
    if (m_bAdoptedByC[ui] && m_uiAlphaDepths[ui] == 1 && m_uiBetaDepths[ui] == 0) // CTU C is included in group alpha
    {
      return true;
    }
  }

  return false;
}

/** initiliaze the array that holds the final depths to evaluate during the CU splitting process
*\returns Void
*/
Void TEncCu::initRangeDepths()
{
  for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
  {
    m_bRangeDepths[ui] = false;
  }
}

/** perform High Similarity degree
*\param   pcCU
*\returns Void
*/
Void TEncCu::performHighSim(TComDataCU* pcCU)
{
  buildGroupBeta(pcCU);
  if (isBetaIncludedInAlpha()) // recheck 
    // only one depth level is adopted
  {
    for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
    {
      if (m_uiAlphaDepths[ui] > 0)
      {
        m_bRangeDepths[ui] = true;
        break;
      }
    }
  }
  else
    // group beta adopts at least one more depth level
  {
    if (m_uiAlphaDepths[m_uhTotalDepth - 2] > 0)
      // the last depth is adopted by alpha, therefore also adopt the depth before last 
    {
      m_bRangeDepths[m_uhTotalDepth - 2] = true;
      m_bRangeDepths[m_uhTotalDepth - 3] = true;
    }
    else
      // the last depth is not adopted by alpha, therefore adopt the succeeding depth 
    {
      for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
      {
        if (m_uiAlphaDepths[ui] > 0)
        {
          m_bRangeDepths[ui] = true;
          m_bRangeDepths[ui + 1] = true;
          break;
        }
      }
    }
  }
}

/** perform Medium High Similarity degree
*\param   pcCU
*\returns Void
*/
Void TEncCu::performMediumHighSim(TComDataCU* pcCU)
{
  buildGroupBeta(pcCU);
  if (isBetaIncludedInAlpha() == true)
  {
    if (isOnlyAdoptedbyC() == true)
    {
      for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
      {
        if (m_bAdoptedByC[ui] == false && m_uiAlphaDepths[ui] > 0) // CTU C is included in group alpha
        {
          m_bRangeDepths[ui] = true;
        }
      }
    }
    else
    {
      for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
      {
        m_bRangeDepths[ui] = m_uiAlphaDepths[ui] > 0;
      }
    }
  }
  else
  {
    for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
    {
      m_bRangeDepths[ui] = m_uiAlphaDepths[ui] > 0;
    }
    UInt uiMaxOccurences = 0;
    UInt uiMaxOccurencesIdx;
    for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)  // Find the most frequent depth that is adopted only by group beta 
    {
      if (m_uiAlphaDepths[ui] == 0 && m_uiBetaDepths[ui] > uiMaxOccurences)
        // only happens if group beta adopts more depths than group alpha
      {
        uiMaxOccurencesIdx = ui;
        uiMaxOccurences = m_uiBetaDepths[ui];
      }
    }
    if (uiMaxOccurences > 0)
      // make sure that group beta adopts more depths than group alpha
    {
      m_bRangeDepths[uiMaxOccurencesIdx] = true;
    }
  }
}

/** perform Medium Low Similarity degree
*\returns Void
*/
Void TEncCu::performMediumLowSim()
{
  Bool adoptedByAllAlphaCUs = false;
  UInt adoptedByAllAlphaCUsIdx;
  Bool adoptedByOneCU = false;
  UInt adoptedByOneCUIdx;


  for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
  {
    if (m_uiAlphaDepths[ui] > 0)
    {
      m_bRangeDepths[ui] = true;
    }
    else
    {
      m_bRangeDepths[ui] = false;
      continue;
    }
    if (m_uiSizeAlpha == 1)
      // if the size of group alpha is 1 (i.e. only the colocated CTU), then all three depth levels possess
      // the same probability (=1). hence, it�s difficult to eliminate one more depth level for the current
      // CTU because the current CTU tends to adopt all three depth levels
    {
      continue;
    }
    else
    {
      if (adoptedByAllAlphaCUs == false)
      {
        adoptedByAllAlphaCUs = m_uiAlphaDepths[ui] == m_uiSizeAlpha ? true : false;
        adoptedByAllAlphaCUsIdx = ui;
      }
      if (m_uiAlphaDepths[ui] == 1)
      {
        if (adoptedByOneCU == false)
        {
          adoptedByOneCU = true;
          adoptedByOneCUIdx = ui;
        }
        if (adoptedByOneCU == true && (abs((Int)adoptedByOneCUIdx - (Int)adoptedByAllAlphaCUsIdx) < abs((Int)ui - (Int)adoptedByAllAlphaCUsIdx)))
        {
          adoptedByOneCUIdx = ui;
        }
      }
    }
  }

  if (adoptedByAllAlphaCUs == true && adoptedByOneCU == true)
  {
    m_bRangeDepths[adoptedByOneCUIdx] = false;
  }
}

/** perform Low Similarity degree
*\returns Void
*/
Void TEncCu::performLowSim()
{
  // Check if there is one depth with lowest probability
  UInt uiLowestProbability = m_uiSizeAlpha;
  UInt uiLowestProbabilityIdx; // saves the first index in the array with lowest probability (low depth)
  for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
  {
    m_bRangeDepths[ui] = true; // initialize
    if (m_uiAlphaDepths[ui] < uiLowestProbability)
    {
      uiLowestProbability = m_uiAlphaDepths[ui];
      uiLowestProbabilityIdx = ui;
    }
  }
  Bool bEqualDepthProbabilities = false;
  UInt uiCommonLowestProbabilityIdx; // saves the last index in the array with the same lowest probability (high depth)
  for (UInt ui = 0; ui < m_uhTotalDepth - 1; ui++)
  {
    if (m_uiAlphaDepths[ui] == uiLowestProbability && ui != uiLowestProbabilityIdx)
    {
      bEqualDepthProbabilities = true;
      uiCommonLowestProbabilityIdx = ui;
    }
  }

  if (bEqualDepthProbabilities == true)
    // at least two depths with the same lowest probability
  {
    UInt uiLow = 0;
    UInt uiHigh = 0;
    uiLow += m_bAdoptedByColocated[0] ? 1 : 0;
    uiLow += m_bAdoptedByColocated[1] ? 1 : 0;
    uiHigh += m_bAdoptedByColocated[2] ? 1 : 0;
    if (m_uhTotalDepth - 1 >= 3)
    {
      uiHigh += m_bAdoptedByColocated[3] ? 1 : 0;
    }
    if (uiLow > uiHigh)
    {
      m_bRangeDepths[uiCommonLowestProbabilityIdx] = false;
    }
    else
    {
      m_bRangeDepths[uiLowestProbabilityIdx] = false;
    }
  }
  else
    // unique depth with lowest probability
  {
    m_bRangeDepths[uiLowestProbabilityIdx] = false;
  }
}

/** RRSP: updates the array that contains booleans whether neighbor CTUs in group A adopt size of 64x64
*\param   pcCu
*\returns Void
*/
Void TEncCu::evaluateGroupA64x64(TComDataCU* pcCU)
{
  // initialization
  for (UInt ui = 0; ui < 4; ui++)
  {
    m_bRRSPAdoptedDepths64x64ByA[ui] = false;
  }
  m_RRSPNumOfCTUsInA = 0;

  TComDataCU* leftCU = pcCU->getCtuLeft();
  if (leftCU != NULL)
  {
    m_bRRSPAdoptedDepths64x64ByA[A] = leftCU->getDepth(0) == 0 ;  // 64x64, no matter which 8x8 CU is picked
    m_RRSPNumOfCTUsInA++;
  }

  TComDataCU* aboveCU = pcCU->getCtuAbove();
  if (aboveCU != NULL)
  {
    m_bRRSPAdoptedDepths64x64ByA[B] = aboveCU->getDepth(0) == 0;  // 64x64, no matter which 8x8 CU is picked
    m_RRSPNumOfCTUsInA++;
  }

  TComDataCU* aboveLeftCU = pcCU->getCtuAboveLeft();
  if (aboveLeftCU != NULL)
  {
    m_bRRSPAdoptedDepths64x64ByA[C] = aboveLeftCU->getDepth(0) == 0;  // 64x64, no matter which 8x8 CU is picked
    m_RRSPNumOfCTUsInA++;
  }

  TComDataCU* colocatedCU = pcCU->getCUColocated(REF_PIC_LIST_0);
  if (colocatedCU != NULL)
  {
    m_bRRSPAdoptedDepths64x64ByA[I] = colocatedCU->getDepth(0) == 0;  // 64x64, no matter which 8x8 CU is picked
    m_RRSPNumOfCTUsInA++;
  }
}


/** RRSP: updates the array that contains booleans whether neighbor CTUs in group B adopt size of 64x64
*\param   pcCu
*\returns Void
*/
Void TEncCu::evaluateGroupB64x64(TComDataCU* pcCU)
{
  // initialization
  for (UInt ui = 0; ui < 5; ui++)
  {
    m_bRRSPAdoptedDepths64x64ByB[ui] = false;
  }
  m_RRSPNumOfCTUsInB = 0;

  TComDataCU* aboveRightCU = pcCU->getCtuAboveRight();
  if (aboveRightCU != NULL)
  {
    m_bRRSPAdoptedDepths64x64ByB[D] = aboveRightCU->getDepth(0) == 0;  // 64x64, no matter which 8x8 CU is picked
    m_RRSPNumOfCTUsInB++;
  }

  TComDataCU* aboveColocatedCU = pcCU->getCUColocated(REF_PIC_LIST_0)->getCtuAbove();
  if (aboveColocatedCU != NULL)
  {
    m_bRRSPAdoptedDepths64x64ByB[E] = aboveColocatedCU->getDepth(0) == 0;  // 64x64, no matter which 8x8 CU is picked
    m_RRSPNumOfCTUsInB++;
  }

  TComDataCU* leftColocatedCU = pcCU->getCUColocated(REF_PIC_LIST_0)->getCtuLeft();
  if (leftColocatedCU != NULL)
  {
    m_bRRSPAdoptedDepths64x64ByB[F] = leftColocatedCU->getDepth(0) == 0;  // 64x64, no matter which 8x8 CU is picked
    m_RRSPNumOfCTUsInB++;
  }

  TComDataCU* aboveLeftColocatedCU = pcCU->getCUColocated(REF_PIC_LIST_0)->getCtuBottom();
  if (aboveLeftColocatedCU != NULL)
  {
    m_bRRSPAdoptedDepths64x64ByB[G] = aboveLeftColocatedCU->getDepth(0) == 0;  // 64x64, no matter which 8x8 CU is picked
    m_RRSPNumOfCTUsInB++;
  }

  TComDataCU* rightColocatedCU = pcCU->getCUColocated(REF_PIC_LIST_0)->getCtuRight();
  if (rightColocatedCU != NULL)
  {
    m_bRRSPAdoptedDepths64x64ByB[H] = rightColocatedCU->getDepth(0) == 0;  // 64x64, no matter which 8x8 CU is picked
    m_RRSPNumOfCTUsInB++;
  }
}

/** RRSP: count how many 32x32 CUs are adopted in group A
*\param   pcCU
*\returns UInt
*/
UInt TEncCu::getNumOf32x32CUsInA(TComDataCU* pcCU)
{
  // initialization
  UInt uiNumOf32x32CUsInA = 0;
  UInt uiDiff = 0;

  Int iRRSPQP = pcCU->getQP(0);
  if (m_pcEncCfg->getUseRateCtrl())
  {
    iRRSPQP = m_pcRateCtrl->getRCQP();
  }

  TComDataCU* leftCU = pcCU->getCtuLeft();
  if (leftCU != NULL)
  {
    uiNumOf32x32CUsInA += leftCU->getDepth(0)   == 1;
    uiNumOf32x32CUsInA += leftCU->getDepth(64)  == 1;
    uiNumOf32x32CUsInA += leftCU->getDepth(128) == 1;
    uiNumOf32x32CUsInA += leftCU->getDepth(192) == 1;
    if (iRRSPQP > 35 && uiNumOf32x32CUsInA - uiDiff == NUM_OF_32X32_CTUS_IN_64X64_CTU)
    {
      uiNumOf32x32CUsInA += NUM_OF_32X32_CTUS_IN_64X64_CTU;
      return uiNumOf32x32CUsInA;
    }
  }

  TComDataCU* aboveCU = pcCU->getCtuAbove();
  if (aboveCU != NULL)
  {
    uiDiff = uiNumOf32x32CUsInA;
    uiNumOf32x32CUsInA += aboveCU->getDepth(0)   == 1;
    uiNumOf32x32CUsInA += aboveCU->getDepth(64)  == 1;
    uiNumOf32x32CUsInA += aboveCU->getDepth(128) == 1;
    uiNumOf32x32CUsInA += aboveCU->getDepth(192) == 1;
    if (iRRSPQP > 35 && uiNumOf32x32CUsInA - uiDiff == NUM_OF_32X32_CTUS_IN_64X64_CTU)
    {
      uiNumOf32x32CUsInA += NUM_OF_32X32_CTUS_IN_64X64_CTU;
      return uiNumOf32x32CUsInA;
    }
  }

  TComDataCU* aboveLeftCU = pcCU->getCtuAboveLeft();
  if (aboveLeftCU != NULL)
  {
    uiDiff = uiNumOf32x32CUsInA;
    uiNumOf32x32CUsInA += aboveLeftCU->getDepth(0)   == 1;
    uiNumOf32x32CUsInA += aboveLeftCU->getDepth(64)  == 1;
    uiNumOf32x32CUsInA += aboveLeftCU->getDepth(128) == 1;
    uiNumOf32x32CUsInA += aboveLeftCU->getDepth(192) == 1;
    if (iRRSPQP > 35 && uiNumOf32x32CUsInA - uiDiff == NUM_OF_32X32_CTUS_IN_64X64_CTU)
    {
      uiNumOf32x32CUsInA += NUM_OF_32X32_CTUS_IN_64X64_CTU;
      return uiNumOf32x32CUsInA;
    }
  }

  TComDataCU* colocatedCU = pcCU->getCUColocated(REF_PIC_LIST_0);
  if (colocatedCU != NULL)
  {
    uiDiff = uiNumOf32x32CUsInA;
    uiNumOf32x32CUsInA += colocatedCU->getDepth(0)   == 1;
    uiNumOf32x32CUsInA += colocatedCU->getDepth(64)  == 1;
    uiNumOf32x32CUsInA += colocatedCU->getDepth(128) == 1;
    uiNumOf32x32CUsInA += colocatedCU->getDepth(192) == 1;
    if (iRRSPQP > 35 && uiNumOf32x32CUsInA - uiDiff == NUM_OF_32X32_CTUS_IN_64X64_CTU)
    {
      uiNumOf32x32CUsInA += NUM_OF_32X32_CTUS_IN_64X64_CTU;
      return uiNumOf32x32CUsInA;
    }
  }

  return uiNumOf32x32CUsInA;
}

/** RRSP: fill the array of depths adopted by CUs in group alpha in relation to the current 32x32 CU
/** Should only be called when performing depth 0 (64x64)
*\param   pcCu
*\param   uiPartUnitIdx
*\returns Void
*/
Void TEncCu::buildRRSPAlphaGroup(TComDataCU* pcCU, RRSP32x32CU uiPartUnitIdx)
{
  UInt uiDepth;
  UInt uiTempDepth;
  UInt uiMultiplier;

  // initialization
  for (UInt ui = 0; ui < m_uhTotalDepth - 2; ui++)
  {
    m_uiRRSPAlphaReducedAdoptedDepths[ui] = 0;
  }

  // 32x32 partition of a CTU
  //  *-----*-----*
  //  |  w  |  x  |
  //  *-----*-----*
  //  |  y  |  z  |
  //  *-----*-----*
  switch (uiPartUnitIdx)
  {
    case w:
    { 
      TComDataCU* leftCU = pcCU->getCtuLeft();
      if (leftCU != NULL)
      {
        for (UInt ui = 84; ui < 148; ui = ui + 32)
        {
          for (UInt uj = 0; uj < 16; uj = uj + 8)
            {
              uiDepth = leftCU->getDepth(ui + uj);
              uiMultiplier = (uiDepth == 0) + 1;
              uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the left CU according to R=8 
              m_uiRRSPAlphaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
            }
          }
        }   
        TComDataCU* aboveLeftCU = pcCU->getCtuAboveLeft();
        if (aboveLeftCU != NULL)
        {
          uiDepth = aboveLeftCU->getDepth(252);
          uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the upper-left CU according to R=8 
          m_uiRRSPAlphaReducedAdoptedDepths[uiTempDepth]++;
        }
        TComDataCU* aboveCU = pcCU->getCtuAbove();
        if (aboveCU != NULL)
        {
          for (UInt ui = 168; ui < 200; ui = ui + 16)
          {
            for (UInt uj = 0; uj < 8; uj = uj + 4)
            {
              uiDepth = aboveCU->getDepth(ui + uj);
              uiMultiplier = (uiDepth == 0) + 1;
              uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the upper CU according to R=8 
              m_uiRRSPAlphaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
            }
          }
        }
        TComDataCU* colocatedCU = pcCU->getCUColocated(REF_PIC_LIST_0);
        if (colocatedCU != NULL)
        {
          for (UInt ui = 0; ui < 64; ui = ui + 4)
          {
            uiDepth = colocatedCU->getDepth(ui);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the colocated CU according to R=8 
            m_uiRRSPAlphaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
        break;
    }
    case x:
    {
      TComDataCU* leftCU = pcCU;
      if (leftCU != NULL)
        {
          for (UInt ui = 20; ui < 84; ui = ui + 32)
          {
            for (UInt uj = 0; uj < 16; uj = uj + 8)
            {
              uiDepth = leftCU->getDepth(ui + uj);
              uiMultiplier = (uiDepth == 0) + 1;
              uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the left CU according to R=8 
              m_uiRRSPAlphaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
            }
          }
        }
        TComDataCU* aboveLeftCU = pcCU->getCtuAbove();
        if (aboveLeftCU != NULL)
        {
          uiDepth = aboveLeftCU->getDepth(188);
          uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the upper-left CU according to R=8
          m_uiRRSPAlphaReducedAdoptedDepths[uiTempDepth]++;
        }
        TComDataCU* aboveCU = aboveLeftCU;
        if (aboveCU != NULL)
        {
          for (UInt ui = 232; ui < 264; ui = ui + 16)
          {
            for (UInt uj = 0; uj < 8; uj = uj + 4)
            {
              uiDepth = aboveCU->getDepth(ui + uj);
              uiMultiplier = (uiDepth == 0) + 1;
              uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the upper CU according to R=8 
              m_uiRRSPAlphaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
            }
          }
        }
        TComDataCU* colocatedCU = pcCU->getCUColocated(REF_PIC_LIST_0);
        if (colocatedCU != NULL)
        {
          for (UInt ui = 64; ui < 128; ui = ui + 4)
          {
            uiDepth = colocatedCU->getDepth(ui);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the colocated CU according to R=8 
            m_uiRRSPAlphaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
        break;
    }
    case y:
    {
      TComDataCU* leftCU = pcCU->getCtuLeft();
      if (leftCU != NULL)
      {
        for (UInt ui = 212; ui < 276; ui = ui + 32)
        {
          for (UInt uj = 0; uj < 16; uj = uj + 8)
          {
            uiDepth = leftCU->getDepth(ui + uj);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the left CU according to R=8 
            m_uiRRSPAlphaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
      }
      TComDataCU* aboveLeftCU = leftCU;
      if (aboveLeftCU != NULL)
      {
        uiDepth = aboveLeftCU->getDepth(124);
        uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the upper-left CU according to R=8 
        m_uiRRSPAlphaReducedAdoptedDepths[uiTempDepth]++;
      }
      TComDataCU* aboveCU = pcCU;
      if (aboveCU != NULL)
      {
        for (UInt ui = 40; ui < 72; ui = ui + 16)
        {
          for (UInt uj = 0; uj < 8; uj = uj + 4)
          {
            uiDepth = aboveCU->getDepth(ui + uj);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the upper CU according to R=8 
            m_uiRRSPAlphaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
      }
      TComDataCU* colocatedCU = pcCU->getCUColocated(REF_PIC_LIST_0);
      if (colocatedCU != NULL)
      {
        for (UInt ui = 128; ui < 192; ui = ui + 4)
        {
          uiDepth = colocatedCU->getDepth(ui);
          uiMultiplier = (uiDepth == 0) + 1;
          uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the colocated CU according to R=8 
          m_uiRRSPAlphaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
        }
      }
      break;
    }
    case z:
    {
      TComDataCU* leftCU = pcCU;
      if (leftCU != NULL)
      {
        for (UInt ui = 148; ui < 212; ui = ui + 32)
        {
         for (UInt uj = 0; uj < 16; uj = uj + 8)
          {
            uiDepth = leftCU->getDepth(ui + uj);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the left CU according to R=8 
            m_uiRRSPAlphaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
      }
      TComDataCU* aboveLeftCU = pcCU;
      if (aboveLeftCU != NULL)
      {
        uiDepth = aboveLeftCU->getDepth(60);
        uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by upper-left CU according to R=8 
        m_uiRRSPAlphaReducedAdoptedDepths[uiTempDepth]++;
      }
      TComDataCU* aboveCU = pcCU;
      if (aboveCU != NULL)
      {
        for (UInt ui = 104; ui < 136; ui = ui + 16)
        {
          for (UInt uj = 0; uj < 8; uj = uj + 4)
          {
            uiDepth = aboveCU->getDepth(ui + uj);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the upper CU according to R=8 
            m_uiRRSPAlphaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
      }
      TComDataCU* colocatedCU = pcCU->getCUColocated(REF_PIC_LIST_0);
      if (colocatedCU != NULL)
      {
        for (UInt ui = 192; ui < 256; ui = ui + 4)
        {
          uiDepth = colocatedCU->getDepth(ui);
          uiMultiplier = (uiDepth == 0) + 1;
          uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the colocated CU according to R=8 
          m_uiRRSPAlphaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
        }
      }
      break;
    }
    default:
    {
      assert(0);
    }
  }
}

/** RRSP: fill the array of depths adopted by CUs in group beta in relation to the current 32x32 CU
/** Should only be called when performing depth 0 (64x64)
*\param   pcCu
*\param   uiPartUnitIdx
*\returns Void
*/
Void TEncCu::buildRRSPBetaGroup(TComDataCU* pcCU, RRSP32x32CU uiPartUnitIdx)
{
  UInt uiDepth;
  UInt uiTempDepth;
  UInt uiMultiplier;

  // initialization
  for (UInt ui = 0; ui < m_uhTotalDepth - 2; ui++)
  {
    m_uiRRSPBetaReducedAdoptedDepths[ui] = 0;
  }

  // 32x32 partition of a CTU
  //  *-----*-----*
  //  |  w  |  x  |
  //  *-----*-----*
  //  |  y  |  z  |
  //  *-----*-----*
  switch (uiPartUnitIdx)
  {
    case w:
    {
      TComDataCU* aboveRightCU = pcCU->getCtuAbove();
      if (aboveRightCU != NULL)
      {
        uiDepth = aboveRightCU->getDepth(232);
        uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the upper-right CU according to R=8
        m_uiRRSPBetaReducedAdoptedDepths[uiTempDepth]++;
      }
      TComDataCU* colocatedAboveCU = pcCU->getCUColocated(REF_PIC_LIST_0)->getCtuAbove();
      if (colocatedAboveCU != NULL)
      {
        for (UInt ui = 168; ui < 200; ui = ui + 16)
        {
          for (UInt uj = 0; uj < 8; uj = uj + 4)
          {
            uiDepth = colocatedAboveCU->getDepth(ui + uj);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the colocated upper CU according to R=8 
            m_uiRRSPBetaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
      }
      TComDataCU* colocatedLeftCU = pcCU->getCUColocated(REF_PIC_LIST_0)->getCtuLeft();
      if (colocatedLeftCU != NULL)
      {
        for (UInt ui = 84; ui < 148; ui = ui + 32)
        {
          for (UInt uj = 0; uj < 16; uj = uj + 8)
          {
            uiDepth = colocatedLeftCU->getDepth(ui + uj);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the colocated left CU according to R=8 
            m_uiRRSPBetaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
      }
      TComDataCU* colocatedBottomCU = pcCU->getCUColocated(REF_PIC_LIST_0);
      if (colocatedBottomCU != NULL)
      {
        for (UInt ui = 128; ui < 160; ui = ui + 16)
        {
          for (UInt uj = 0; uj < 8; uj = uj + 4)
          {
            uiDepth = colocatedBottomCU->getDepth(ui + uj);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the colocated bottom CU according to R=8 
            m_uiRRSPBetaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
      }
      TComDataCU* colocatedRightCU = colocatedBottomCU;
      if (colocatedRightCU != NULL)
      {
        for (UInt ui = 64; ui < 128; ui = ui + 32)
        {
          for (UInt uj = 0; uj < 16; uj = uj + 8)
          {
            uiDepth = colocatedRightCU->getDepth(ui + uj);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the colocated right CU according to R=8 
            m_uiRRSPBetaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
      }
      break;
    }
    case x:
    {
      TComDataCU* aboveRightCU = pcCU->getCtuAboveRight();
      if (aboveRightCU != NULL)
      {
        uiDepth = aboveRightCU->getDepth(168);
        uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the upper-right CU according to R=8
        m_uiRRSPBetaReducedAdoptedDepths[uiTempDepth]++;
      }
      TComDataCU* colocatedAboveCU = pcCU->getCUColocated(REF_PIC_LIST_0)->getCtuAbove();
      if (colocatedAboveCU != NULL)
      {
        for (UInt ui = 232; ui < 264; ui = ui + 16)
        {
          for (UInt uj = 0; uj < 8; uj = uj + 4)
          {
            uiDepth = colocatedAboveCU->getDepth(ui + uj);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the colocated upper CU according to R=8 
            m_uiRRSPBetaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
      }
      TComDataCU* colocatedLeftCU = pcCU->getCUColocated(REF_PIC_LIST_0);
      if (colocatedLeftCU != NULL)
      {
        for (UInt ui = 20; ui < 84; ui = ui + 32)
        {
          for (UInt uj = 0; uj < 16; uj = uj + 8)
          {
            uiDepth = colocatedLeftCU->getDepth(ui + uj);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the colocated left CU according to R=8 
            m_uiRRSPBetaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
      }
      TComDataCU* colocatedBottomCU = colocatedLeftCU;
      if (colocatedBottomCU != NULL)
      {
        for (UInt ui = 192; ui < 224; ui = ui + 16)
        {
          for (UInt uj = 0; uj < 8; uj = uj + 4)
          {
            uiDepth = colocatedBottomCU->getDepth(ui + uj);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the colocated bottom CU according to R=8 
            m_uiRRSPBetaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
      }
      TComDataCU* colocatedRightCU = pcCU->getCUColocated(REF_PIC_LIST_0)->getCtuRight();
      if (colocatedRightCU != NULL)
      {
        for (UInt ui = 0; ui < 64; ui = ui + 32)
        {
          for (UInt uj = 0; uj < 16; uj = uj + 8)
          {
            uiDepth = colocatedRightCU->getDepth(ui + uj);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the colocated right CU according to R=8 
            m_uiRRSPBetaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
      }
      break;
    }
    case y:
    {
      TComDataCU* aboveRightCU = pcCU;
      if (aboveRightCU != NULL)
      {
        uiDepth = aboveRightCU->getDepth(104);
        uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the upper-right CU according to R=8
        m_uiRRSPBetaReducedAdoptedDepths[uiTempDepth]++;
      }
      TComDataCU* colocatedAboveCU = pcCU->getCUColocated(REF_PIC_LIST_0);
      if (colocatedAboveCU != NULL)
      {
        for (UInt ui = 40; ui < 72; ui = ui + 16)
        {
          for (UInt uj = 0; uj < 8; uj = uj + 4)
          {
            uiDepth = colocatedAboveCU->getDepth(ui + uj);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the colocated upper CU according to R=8 
            m_uiRRSPBetaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
      }
      TComDataCU* colocatedLeftCU = pcCU->getCUColocated(REF_PIC_LIST_0)->getCtuLeft();
      if (colocatedLeftCU != NULL)
      {
        for (UInt ui = 212; ui < 276; ui = ui + 32)
        {
          for (UInt uj = 0; uj < 16; uj = uj + 8)
          {
            uiDepth = colocatedLeftCU->getDepth(ui + uj);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the colocated left CU according to R=8 
            m_uiRRSPBetaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
      }
      TComDataCU* colocatedBottomCU = pcCU->getCUColocated(REF_PIC_LIST_0)->getCtuBottom();
      if (colocatedBottomCU != NULL)
      {
        for (UInt ui = 0; ui < 32; ui = ui + 16)
        {
          for (UInt uj = 0; uj < 8; uj = uj + 4)
          {
            uiDepth = colocatedBottomCU->getDepth(ui + uj);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the colocated bottom CU according to R=8 
            m_uiRRSPBetaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
      }
      TComDataCU* colocatedRightCU = colocatedAboveCU;
      if (colocatedRightCU != NULL)
      {
        for (UInt ui = 192; ui < 256; ui = ui + 32)
        {
          for (UInt uj = 0; uj < 16; uj = uj + 8)
          {
            uiDepth = colocatedRightCU->getDepth(ui + uj);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the colocated right CU according to R=8 
            m_uiRRSPBetaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
      }
      break;
    }
    case z:
    {
      TComDataCU* colocatedAboveCU = pcCU->getCUColocated(REF_PIC_LIST_0);
      if (colocatedAboveCU != NULL)
      {
        for (UInt ui = 104; ui < 136; ui = ui + 16)
        {
          for (UInt uj = 0; uj < 8; uj = uj + 4)
          {
            uiDepth = colocatedAboveCU->getDepth(ui + uj);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the colocated upper CU according to R=8 
            m_uiRRSPBetaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
      }
      TComDataCU* colocatedLeftCU = colocatedAboveCU;
      if (colocatedLeftCU != NULL)
      {
        for (UInt ui = 148; ui < 212; ui = ui + 32)
        {
          for (UInt uj = 0; uj < 16; uj = uj + 8)
          {
            uiDepth = colocatedLeftCU->getDepth(ui + uj);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the colocated left CU according to R=8 
            m_uiRRSPBetaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
      }
      TComDataCU* colocatedBottomCU = pcCU->getCUColocated(REF_PIC_LIST_0)->getCtuBottom();
      if (colocatedBottomCU != NULL)
      {
        for (UInt ui = 64; ui < 96; ui = ui + 16)
        {
          for (UInt uj = 0; uj < 8; uj = uj + 4)
          {
            uiDepth = colocatedBottomCU->getDepth(ui + uj);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the colocated bottom CU according to R=8 
            m_uiRRSPBetaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
      }
      TComDataCU* colocatedRightCU = pcCU->getCUColocated(REF_PIC_LIST_0)->getCtuRight();
      if (colocatedRightCU != NULL)
      {
        for (UInt ui = 128; ui < 192; ui = ui + 32)
        {
          for (UInt uj = 0; uj < 16; uj = uj + 8)
          {
            uiDepth = colocatedRightCU->getDepth(ui + uj);
            uiMultiplier = (uiDepth == 0) + 1;
            uiTempDepth = (uiDepth == 0) ? 0 : uiDepth - 1; // indicates the depth adopted by the colocated right CU according to R=8 
            m_uiRRSPBetaReducedAdoptedDepths[uiTempDepth] += uiMultiplier;
          }
        }
      }
      break;
    }
    default:
    {
      assert(0);
    }
  }
}

/** RRSP: calculate and return the RRSP similarity level
*\returns UInt
*/
UInt TEncCu::getRRSPSimLevel()
{
  // simLevel = 0 if all 32x32 and 16x16 and 8x8 are adopted. 
  // simLevel = 2 if just one of the above is adopted.
  UInt simLevel = 0;
  for (UInt ui = 0; ui < m_uhTotalDepth - 2; ui++)
  {
    simLevel += (m_uiRRSPAlphaReducedAdoptedDepths[ui] == 0) ? 1 : 0;
  }
  return simLevel;
}

/** RRSP: set the final search range in the RRSP algorithm
*\param   simLevel
*\param   pcCU
*\param   uiPartUnitIdx
*\returns UInt
*/
Void TEncCu::setReducedRangeDepths(UInt simLevel, TComDataCU* pcCU, RRSP32x32CU uiPartUnitIdx)
{
  // initialization
  for (UInt ui = 0; ui < m_uhTotalDepth - 2; ui++)
  {
    m_bReducedRangeDepths[ui] = false;
  }

  switch (simLevel)
  {
    case LOW:
    {
      performRRSPLowSim(pcCU, uiPartUnitIdx);
      break;
    }
    case MEDIUM:
    {
      performRRSPMediumSim(pcCU, uiPartUnitIdx);
      break;
    }
    case HIGH:
    {
      performRRSPHighSim(pcCU, uiPartUnitIdx);
      break;
    }
    default:
    {
      assert(0);
    }
  }
}

/** RRSP: perform Low Similarity degree
*\param   pcCU
*\param   uiPartUnitIdx
*\returns Void
*/
Void TEncCu::performRRSPLowSim(TComDataCU* pcCU, RRSP32x32CU uiPartUnitIdx)
{
  // initialization
  for (UInt ui = 0; ui < m_uhTotalDepth - 2; ui++)
  {
    m_bReducedRangeDepths[ui] = true;
  }
  
  // if possible, disable one depth with low probability
  if (m_uiRRSPAlphaReducedAdoptedDepths[0] > (NUM_OF_8X8_CTUS_IN_32X32_CTU + CU_32X32_IS_ADOPTED_IN_FOURSOMES))
    // this is a vast majority out of twenty five possible 8x8 CUs in group alpha.
    // since there are two other depths which are also adopted, it basically means that the depth which holds the majority is adopted
    // by exactly 21 CUs - the colocated 32x32 CU (16), CU c (1), and one of the CUs a or b (4).
    // note that if group alpha consists of less than 4 CTUs, only sixteen or twenty 8x8 CUs are available - hence this condition is never true
  {
    m_bReducedRangeDepths[m_uhTotalDepth - 3] = false;
  }
  else if (m_uiRRSPAlphaReducedAdoptedDepths[m_uhTotalDepth - 3] > (NUM_OF_8X8_CTUS_IN_32X32_CTU + 2 * CU_8X8_IS_ADOPTED_IN_PAIRS))
  {
    m_bReducedRangeDepths[0] = false;
  }
  else if (m_uiRRSPAlphaReducedAdoptedDepths[m_uhTotalDepth - 3] <= CU_8X8_IS_ADOPTED_IN_PAIRS)
  {
    if (m_uiRRSPAlphaReducedAdoptedDepths[m_uhTotalDepth - 3] == 1)
      // the depth is adopted only by CU c
    {
      m_bReducedRangeDepths[m_uhTotalDepth - 3] = false;
    }
    else
      // deploy group beta to decide if the depth should be eliminated
    {
      buildRRSPBetaGroup(pcCU, uiPartUnitIdx);
      if (m_uiRRSPBetaReducedAdoptedDepths[m_uhTotalDepth - 3] <= CU_8X8_IS_ADOPTED_IN_PAIRS)
      {
        m_bReducedRangeDepths[m_uhTotalDepth - 3] = false;
      }
    }
  }
  else if (m_uiRRSPAlphaReducedAdoptedDepths[0] < CU_32X32_IS_ADOPTED_IN_FOURSOMES)
  {
    // the depth is adopted only by CU c in group alpha
    // deploy group beta to decide if the depth should be eliminated
    buildRRSPBetaGroup(pcCU, uiPartUnitIdx);
    if (m_uiRRSPBetaReducedAdoptedDepths[0] == 0)
      // the depth is not adopted by any CU in group beta
    {
      m_bReducedRangeDepths[0] = false;
    }
  }
}

/** RRSP: perform Medium Similarity degree
*\param   pcCU
*\param   uiPartUnitIdx
*\returns Void
*/
Void TEncCu::performRRSPMediumSim(TComDataCU* pcCU, RRSP32x32CU uiPartUnitIdx)
{
  buildRRSPBetaGroup(pcCU, uiPartUnitIdx);

  Bool bIsBetaAdoptsMoreThanAlpha = false;
  UInt uiExtraDepth;
  Int depthAdoptedByc = -1;

  for (UInt ui = 0; ui < m_uhTotalDepth - 2; ui++)
  {
    m_bReducedRangeDepths[ui] = m_uiRRSPAlphaReducedAdoptedDepths[ui] > 0;
    if (m_uiRRSPAlphaReducedAdoptedDepths[ui] == 1)
      // must be only adopted by CU c. All other CUs adopt each depth in groups of 2 or more
    {
      depthAdoptedByc = ui;
    }
    if (m_uiRRSPAlphaReducedAdoptedDepths[ui] == 0)
    {
      uiExtraDepth = ui;
      bIsBetaAdoptsMoreThanAlpha = m_uiRRSPBetaReducedAdoptedDepths[ui] > 0;
    }
  }

  if (m_uiRRSPAlphaReducedAdoptedDepths[0] > 1 && m_uiRRSPAlphaReducedAdoptedDepths[m_uhTotalDepth - 3] >= NUM_OF_8X8_CTUS_IN_32X32_CTU)
    // 32x32 is not adopted only by CU c and 8x8 CUs are the majority
  {
    m_bReducedRangeDepths[1] = true;
    return;
  }
  
  if (bIsBetaAdoptsMoreThanAlpha)
  {
    UInt adoptedByAtLeast;
    switch (uiExtraDepth)
    {
      case 0:
      {
        adoptedByAtLeast = CU_32X32_IS_ADOPTED_IN_FOURSOMES;
        break;
      }
      case 1:
      {
        adoptedByAtLeast = CU_8X8_IS_ADOPTED_IN_PAIRS;
        break;
      }
      case 2:
      {
        adoptedByAtLeast = CU_8X8_IS_ADOPTED_IN_PAIRS;
        break;
      }
      default:
      {
        assert(0);
      }
    }
    if (m_uiRRSPBetaReducedAdoptedDepths[uiExtraDepth] > adoptedByAtLeast)  // this depth must be adopted by more than "adoptedByAtLeast" 8x8 CUs in order to be evaluated
    {
      m_bReducedRangeDepths[uiExtraDepth] = true;
    }
  }
  else
  {
    if (depthAdoptedByc != -1)
    {
      if (m_uiRRSPBetaReducedAdoptedDepths[depthAdoptedByc] == 0) // the depth is only adopted by CU c
      {
        m_bReducedRangeDepths[depthAdoptedByc] = false;
      }
    }
  }
}

/** RRSP: perform High Similarity degree
*\param   pcCU
*\param   uiPartUnitIdx
*\returns Void
*/
Void TEncCu::performRRSPHighSim(TComDataCU* pcCU, RRSP32x32CU uiPartUnitIdx)
{
  buildRRSPBetaGroup(pcCU, uiPartUnitIdx);

  UInt uiDepthAdoptedByAlpha;
  Bool bIsBetaAdoptsMoreThanAlpha = false;
  static Bool isGrandfatherFrameInUse = false;

  for (UInt ui = 0; ui < m_uhTotalDepth - 2; ui++)
  {
    m_bReducedRangeDepths[ui] = m_uiRRSPAlphaReducedAdoptedDepths[ui] > 0;
    if (m_uiRRSPAlphaReducedAdoptedDepths[ui] > 0)
    {
      uiDepthAdoptedByAlpha = ui;
    }
    else if (m_uiRRSPBetaReducedAdoptedDepths[ui] > 0)
    {
      bIsBetaAdoptsMoreThanAlpha = true;
    }
  }

  if (bIsBetaAdoptsMoreThanAlpha)
    // group beta adopts at least one more depth that is not adopted by group alpha
  {
    if (uiDepthAdoptedByAlpha < m_uhTotalDepth - 3)
      // group alpha adopts 32x32 or 16x16
    {
      m_bReducedRangeDepths[uiDepthAdoptedByAlpha + 1] = true;
    }
    else
      // group alpha adopts 8x8
    {
      m_bReducedRangeDepths[uiDepthAdoptedByAlpha - 1] = true;
    }
  }
  else
    // groups alpha and beta adopt the same single depth
  {
    Int iRRSPQP = pcCU->getQP(0);
    if (m_pcEncCfg->getUseRateCtrl())
    {
      iRRSPQP = m_pcRateCtrl->getRCQP();
    }
    if (iRRSPQP <= 35)
      // higher QPs tend to adopt the same CU sizes, hence skip using previous frames
    {
      if (pcCU->getCUColocated(REF_PIC_LIST_0)->getCUColocated(REF_PIC_LIST_0) != NULL)
        // the current frame is at least the third frame
      {
        if (!isGrandfatherFrameInUse)
          // use only once, otherwise in an homogenous region a recursive loop might return to the start of the sequence 
        {
          isGrandfatherFrameInUse = true;
          buildRRSPAlphaGroup(pcCU->getCUColocated(REF_PIC_LIST_0), uiPartUnitIdx);
          setReducedRangeDepths(getRRSPSimLevel(), pcCU->getCUColocated(REF_PIC_LIST_0), uiPartUnitIdx);
          isGrandfatherFrameInUse = false; // reset
          if (uiDepthAdoptedByAlpha > 0)
          {
            m_bReducedRangeDepths[uiDepthAdoptedByAlpha - 1] = true; // similar smaller CUs might combine into a larger CU
          }
        }
      }
      else if (!isGrandfatherFrameInUse && uiDepthAdoptedByAlpha > 0)
        // evaluating the second frame
      {
        // similar smaller CUs might combine into a larger CU
        m_bReducedRangeDepths[uiDepthAdoptedByAlpha - 1] = true;
      }
    }
    else
    {
      if (uiDepthAdoptedByAlpha > 0)
      {
        // similar smaller CUs tend to combine into a larger CU
        m_bReducedRangeDepths[uiDepthAdoptedByAlpha - 1] = true;
      }
    }
  }
}

//! \}