/* �����R�[�h�͂r�i�h�r ���s�R�[�h�͂b�q�k�e */
/* $Id$ */

#include "StdAfx.h"
#include "utvideo.h"
#include "ULRGCodec.h"
#include "Predict.h"
#include "MediaSubType.h"

const utvf_t CULRGCodec::m_utvfEncoderInput[] = {
	UTVF_RGB24_WIN,
	UTVF_RGB32_WIN,
	UTVF_INVALID,
};

const utvf_t CULRGCodec::m_utvfDecoderOutput[] = {
	UTVF_RGB24_WIN,
	UTVF_RGB32_WIN,
	UTVF_INVALID,
};

const utvf_t CULRGCodec::m_utvfCompressed[] = {
	UTVF_ULRG,
	UTVF_INVALID,
};

CULRGCodec::CULRGCodec(const char *pszInterfaceName) : CUL00Codec(pszInterfaceName)
{
}

CULRGCodec::~CULRGCodec(void)
{
}

CCodec *CULRGCodec::CreateInstance(const char *pszInterfaceName)
{
	return new CULRGCodec(pszInterfaceName);
}

void CULRGCodec::CalcPlaneSizes(unsigned int width, unsigned int height)
{
	m_dwPlaneSize[0]          = width * height;
	m_dwPlaneSize[1]          = width * height;
	m_dwPlaneSize[2]          = width * height;

	m_dwPlaneWidth[0]         = width;
	m_dwPlaneWidth[1]         = width;
	m_dwPlaneWidth[2]         = width;

	m_dwPlaneStripeSize[0]    = width;
	m_dwPlaneStripeSize[1]    = width;
	m_dwPlaneStripeSize[2]    = width;

	m_dwPlanePredictStride[0] = width;
	m_dwPlanePredictStride[1] = width;
	m_dwPlanePredictStride[2] = width;
}

void CULRGCodec::ConvertToPlanar(DWORD nBandIndex)
{
	BYTE *g, *b, *r;
	const BYTE *pSrcBegin, *pSrcEnd, *pStrideBegin, *p;

	pSrcBegin = ((BYTE *)m_pInput) + m_dwRawStripeBegin[nBandIndex] * m_dwRawStripeSize;
	pSrcEnd   = ((BYTE *)m_pInput) + m_dwRawStripeEnd[nBandIndex]   * m_dwRawStripeSize;
	g = m_pCurFrame->GetPlane(0) + m_dwPlaneStripeBegin[nBandIndex] * m_dwPlaneStripeSize[0];
	b = m_pCurFrame->GetPlane(1) + m_dwPlaneStripeBegin[nBandIndex] * m_dwPlaneStripeSize[1];
	r = m_pCurFrame->GetPlane(2) + m_dwPlaneStripeBegin[nBandIndex] * m_dwPlaneStripeSize[2];

	switch (m_utvfRaw)
	{
	case UTVF_RGB24_WIN:
		for (pStrideBegin = pSrcEnd - m_dwRawGrossWidth; pStrideBegin >= pSrcBegin; pStrideBegin -= m_dwRawGrossWidth)
		{
			const BYTE *pStrideEnd = pStrideBegin + m_nWidth * 3;
			for (p = pStrideBegin; p < pStrideEnd; p += 3)
			{
				*g++ = *(p+1);
				*b++ = *(p+0) - *(p+1) + 0x80;
				*r++ = *(p+2) - *(p+1) + 0x80;
			}
		}
		break;
	case UTVF_RGB32_WIN:
		for (pStrideBegin = pSrcEnd - m_dwRawGrossWidth; pStrideBegin >= pSrcBegin; pStrideBegin -= m_dwRawGrossWidth)
		{
			const BYTE *pStrideEnd = pStrideBegin + m_nWidth * 4;
			for (p = pStrideBegin; p < pStrideEnd; p += 4)
			{
				*g++ = *(p+1);
				*b++ = *(p+0) - *(p+1) + 0x80;
				*r++ = *(p+2) - *(p+1) + 0x80;
			}
		}
		break;
	}
}

void CULRGCodec::ConvertFromPlanar(DWORD nBandIndex)
{
	const BYTE *g, *b, *r;
	BYTE *pDstBegin, *pDstEnd, *pStrideBegin, *p;

	pDstBegin = ((BYTE *)m_pOutput) + m_dwRawStripeBegin[nBandIndex] * m_dwRawStripeSize;
	pDstEnd   = ((BYTE *)m_pOutput) + m_dwRawStripeEnd[nBandIndex]   * m_dwRawStripeSize;
	g = m_pCurFrame->GetPlane(0) + m_dwPlaneStripeBegin[nBandIndex] * m_dwPlaneStripeSize[0];
	b = m_pCurFrame->GetPlane(1) + m_dwPlaneStripeBegin[nBandIndex] * m_dwPlaneStripeSize[1];
	r = m_pCurFrame->GetPlane(2) + m_dwPlaneStripeBegin[nBandIndex] * m_dwPlaneStripeSize[2];

	switch (m_utvfRaw)
	{
	case UTVF_RGB24_WIN:
		for (pStrideBegin = pDstEnd - m_dwRawGrossWidth; pStrideBegin >= pDstBegin; pStrideBegin -= m_dwRawGrossWidth)
		{
			BYTE *pStrideEnd = pStrideBegin + m_nWidth * 3;
			for (p = pStrideBegin; p < pStrideEnd; p += 3)
			{
				*(p+1) = *g;
				*(p+0) = *b + *g - 0x80;
				*(p+2) = *r + *g - 0x80;
				g++; b++; r++;
			}
		}
		break;
	case UTVF_RGB32_WIN:
		for (pStrideBegin = pDstEnd - m_dwRawGrossWidth; pStrideBegin >= pDstBegin; pStrideBegin -= m_dwRawGrossWidth)
		{
			BYTE *pStrideEnd = pStrideBegin + m_nWidth * 4;
			for (p = pStrideBegin; p < pStrideEnd; p += 4)
			{
				*(p+1) = *g;
				*(p+0) = *b + *g - 0x80;
				*(p+2) = *r + *g - 0x80;
				*(p+3) = 0xff;
				g++; b++; r++;
			}
		}
		break;
	}
}

BOOL CULRGCodec::DecodeDirect(DWORD nBandIndex)
{
	if ((m_fi.dwFlags0 & FI_FLAGS0_INTRAFRAME_PREDICT_MASK) != FI_FLAGS0_INTRAFRAME_PREDICT_LEFT)
		return FALSE;

	BYTE *pDstBegin = ((BYTE *)m_pOutput) + m_dwRawStripeBegin[nBandIndex] * m_dwRawStripeSize;
	BYTE *pDstEnd   = ((BYTE *)m_pOutput) + m_dwRawStripeEnd[nBandIndex]   * m_dwRawStripeSize;

	switch (m_utvfRaw)
	{
	case UTVF_RGB24_WIN:
		HuffmanDecodeAndAccumStep3ForBottomupRGB24Green(pDstBegin+1, pDstEnd+1, m_pDecodeCode[0][nBandIndex], &m_hdt[0], m_dwRawNetWidth, m_dwRawGrossWidth);
		HuffmanDecodeAndAccumStep3ForBottomupRGB24Blue (pDstBegin+0, pDstEnd+0, m_pDecodeCode[1][nBandIndex], &m_hdt[1], m_dwRawNetWidth, m_dwRawGrossWidth);
		HuffmanDecodeAndAccumStep3ForBottomupRGB24Red  (pDstBegin+2, pDstEnd+2, m_pDecodeCode[2][nBandIndex], &m_hdt[2], m_dwRawNetWidth, m_dwRawGrossWidth);
		return TRUE;
	case UTVF_RGB32_WIN:
		HuffmanDecodeAndAccumStep4ForBottomupRGB32Green           (pDstBegin+1, pDstEnd+1, m_pDecodeCode[0][nBandIndex], &m_hdt[0], m_dwRawNetWidth, m_dwRawGrossWidth);
		HuffmanDecodeAndAccumStep4ForBottomupRGB32Blue            (pDstBegin+0, pDstEnd+0, m_pDecodeCode[1][nBandIndex], &m_hdt[1], m_dwRawNetWidth, m_dwRawGrossWidth);
		HuffmanDecodeAndAccumStep4ForBottomupRGB32RedAndDummyAlpha(pDstBegin+2, pDstEnd+2, m_pDecodeCode[2][nBandIndex], &m_hdt[2], m_dwRawNetWidth, m_dwRawGrossWidth);
		return TRUE;
	}

	return FALSE;
}