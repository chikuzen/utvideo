/* �����R�[�h�͂r�i�h�r ���s�R�[�h�͂b�q�k�e */
/* $Id$ */

#include "stdafx.h"
#include "utvideo.h"
#include "TunedFunc.h"
#include "Predict.h"
#include "HuffmanCode.h"
#include "Convert.h"

const TUNEDFUNC tfnSSE2 = {
	sse2_PredictWrongMedianAndCount_align16,
	sse2_PredictWrongMedianAndCount_align1,
	sse2_PredictLeftAndCount_align1,
	sse1mmx_RestoreWrongMedian_align1,
	i686_HuffmanEncode,
	i686_HuffmanDecode,
	i686_HuffmanDecodeAndAccum,
	i686_HuffmanDecodeAndAccumStep2,
	i686_HuffmanDecodeAndAccumStep4,
	i686_HuffmanDecodeAndAccumStep4ForBottomupBGRXGreen,
	i686_HuffmanDecodeAndAccumStep4ForBottomupBGRXBlue,
	i686_HuffmanDecodeAndAccumStep4ForBottomupBGRXRed,
	i686_HuffmanDecodeAndAccumStep4ForBottomupBGRXRedAndDummyAlpha,
	i686_HuffmanDecodeAndAccumStep3ForBottomupBGRGreen,
	i686_HuffmanDecodeAndAccumStep3ForBottomupBGRBlue,
	i686_HuffmanDecodeAndAccumStep3ForBottomupBGRRed,
	i686_HuffmanDecodeAndAccumStep4ForTopdownXRGBGreen,
	i686_HuffmanDecodeAndAccumStep4ForTopdownXRGBBlue,
	i686_HuffmanDecodeAndAccumStep4ForTopdownXRGBRed,
	i686_HuffmanDecodeAndAccumStep4ForTopdownXRGBRedAndDummyAlpha,
	i686_HuffmanDecodeAndAccumStep3ForTopdownRGBGreen,
	i686_HuffmanDecodeAndAccumStep3ForTopdownRGBBlue,
	i686_HuffmanDecodeAndAccumStep3ForTopdownRGBRed,
	i686_HuffmanDecodeAndAccumStep4ForTopdownBGRXRedAndDummyAlpha,
	sse2_ConvertULY2ToBGR,
	sse2_ConvertULY2ToBGRX,
	sse2_ConvertULY2ToRGB,
	sse2_ConvertULY2ToXRGB,
	sse2_ConvertBGRToULY2,
	sse2_ConvertBGRXToULY2,
	sse2_ConvertRGBToULY2,
	sse2_ConvertXRGBToULY2,
	cpp_ConvertBGRToULRG,
	cpp_ConvertBGRXToULRG,
	cpp_ConvertXRGBToULRG,
	cpp_ConvertBGRAToULRA,
	cpp_ConvertARGBToULRA,
	cpp_ConvertYUYVToULY2,
	cpp_ConvertUYVYToULY2,
	cpp_ConvertULRGToBGR,
	cpp_ConvertULRGToBGRX,
	cpp_ConvertULRGToXRGB,
	cpp_ConvertULRAToBGRA,
	cpp_ConvertULRAToARGB,
	cpp_ConvertULY2ToYUYV,
	cpp_ConvertULY2ToUYVY,
	DummyTunedFunc
};

const TUNEDFUNC &tfnSSE3 = tfnSSE2;

const TUNEDFUNC &tfnSSSE3 = tfnSSE2;

const TUNEDFUNC &tfnSSE41 = tfnSSE2;

const TUNEDFUNC &tfnSSE42 = tfnSSE2;

const TUNEDFUNC tfnAVX1 = {
	sse2_PredictWrongMedianAndCount_align16,
	sse2_PredictWrongMedianAndCount_align1,
	sse2_PredictLeftAndCount_align1,
	sse1mmx_RestoreWrongMedian_align1,
	i686_HuffmanEncode,
	i686_HuffmanDecode,
	i686_HuffmanDecodeAndAccum,
	i686_HuffmanDecodeAndAccumStep2,
	i686_HuffmanDecodeAndAccumStep4,
	i686_HuffmanDecodeAndAccumStep4ForBottomupBGRXGreen,
	i686_HuffmanDecodeAndAccumStep4ForBottomupBGRXBlue,
	i686_HuffmanDecodeAndAccumStep4ForBottomupBGRXRed,
	i686_HuffmanDecodeAndAccumStep4ForBottomupBGRXRedAndDummyAlpha,
	i686_HuffmanDecodeAndAccumStep3ForBottomupBGRGreen,
	i686_HuffmanDecodeAndAccumStep3ForBottomupBGRBlue,
	i686_HuffmanDecodeAndAccumStep3ForBottomupBGRRed,
	i686_HuffmanDecodeAndAccumStep4ForTopdownXRGBGreen,
	i686_HuffmanDecodeAndAccumStep4ForTopdownXRGBBlue,
	i686_HuffmanDecodeAndAccumStep4ForTopdownXRGBRed,
	i686_HuffmanDecodeAndAccumStep4ForTopdownXRGBRedAndDummyAlpha,
	i686_HuffmanDecodeAndAccumStep3ForTopdownRGBGreen,
	i686_HuffmanDecodeAndAccumStep3ForTopdownRGBBlue,
	i686_HuffmanDecodeAndAccumStep3ForTopdownRGBRed,
	i686_HuffmanDecodeAndAccumStep4ForTopdownBGRXRedAndDummyAlpha,
	sse2_ConvertULY2ToBGR,
	sse2_ConvertULY2ToBGRX,
	sse2_ConvertULY2ToRGB,
	sse2_ConvertULY2ToXRGB,
	sse2_ConvertBGRToULY2,
	sse2_ConvertBGRXToULY2,
	sse2_ConvertRGBToULY2,
	sse2_ConvertXRGBToULY2,
	avx1_ConvertBGRToULRG,
	avx1_ConvertBGRXToULRG,
	avx1_ConvertXRGBToULRG,
	avx1_ConvertBGRAToULRA,
	avx1_ConvertARGBToULRA,
	avx1_ConvertYUYVToULY2,
	avx1_ConvertUYVYToULY2,
	avx1_ConvertULRGToBGR,
	avx1_ConvertULRGToBGRX,
	avx1_ConvertULRGToXRGB,
	avx1_ConvertULRAToBGRA,
	avx1_ConvertULRAToARGB,
	avx1_ConvertULY2ToYUYV,
	avx1_ConvertULY2ToUYVY,
	DummyTunedFunc
};

const TUNEDFUNC &tfnAVX2 = tfnAVX1;

#include "TunedFunc_x86x64.cpp"
