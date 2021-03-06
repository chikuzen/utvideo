/* 文字コードはＳＪＩＳ 改行コードはＣＲＬＦ */
/* $Id$ */

#include "stdafx.h"
#include "utvideo.h"
#include "TunedFunc.h"
#include "TunedFunc_x86x64.h"
#include "Predict.h"
#include "Predict_x86x64.h"
#include "HuffmanCode.h"
#include "Convert.h"
#include "Convert_x86x64.h"
#include "ColorOrder.h"
#include "Coefficient.h"
#include "CPUID.h"
#include "Log.h"

#define FEATURE0_SSE2     (1 <<  0)
#define FEATURE0_SSE3     (1 <<  1)
#define FEATURE0_SSSE3    (1 <<  2)
#define FEATURE0_SSE41    (1 <<  3)
#define FEATURE0_SSE42    (1 <<  4)
#define FEATURE0_AVX1     (1 <<  5)
#define FEATURE0_AVX2     (1 <<  6)
#define FEATURE0_FMA3     (1 <<  7) /* 3-operand FMA */
//#define FEATURE0_AVX512F  (1 <<  8) /* AVX-512 Foundation */
//#define FEATURE0_AVX512PF (1 <<  9) /* AVX-512 Prefetch */
//#define FEATURE0_AVX512ER (1 << 10) /* AVX-512 Exponential & Reciprocal */
//#define FEATURE0_AVX512CD (1 << 11) /* AVX-512 Conflict Detection */

#define FEATURE1_MOVBE    (1 <<  0)
#define FEATURE1_POPCNT   (1 <<  1)
#define FEATURE1_LZCNT    (1 <<  2)
#define FEATURE1_BMI1     (1 <<  3)
#define FEATURE1_BMI2     (1 <<  4)


#define MA_INVALID            0x00000000
#define MA_UNKNOWN            0xffffffff

#define MA_INTEL_MEROM        0x00010000
#define MA_INTEL_PENRYN       0x00010001
#define MA_INTEL_NEHALEM      0x00010002
#define MA_INTEL_WESTMERE     0x00010003
#define MA_INTEL_SANDY_BRIDGE 0x00010004
#define MA_INTEL_IVY_BRIDGE   0x00010005
#define MA_INTEL_HASWELL      0x00010006
#define MA_INTEL_BROADWELL    0x00010007
#define MA_INTEL_SKYLAKE      0x00010008

struct MAFM
{
	const char *name;
	uint32_t ma;
	uint16_t fm[6];
};

/*
 * Reference:
 *   - http://software.intel.com/en-us/articles/intel-architecture-and-processor-identification-with-cpuid-model-and-family-numbers
 * and
 *   - Intel 64 and IA-32 Architectures Optimization Reference Manual, Appendix C
 */
static const struct MAFM mafm[] = {
	{ "Merom",        MA_INTEL_MEROM,        { 0x060f, 0x0616 } },
	{ "Penryn",       MA_INTEL_PENRYN,       { 0x0617, 0x061d } },
	{ "Nehalem",      MA_INTEL_NEHALEM,      { 0x061a, 0x061e, 0x061f, 0x062e } },
	{ "Westmere",     MA_INTEL_WESTMERE,     { 0x0625, 0x062c, 0x062f } },
	{ "Sandy Bridge", MA_INTEL_SANDY_BRIDGE, { 0x062a, 0x062d } },
	{ "Ivy Bridge",   MA_INTEL_IVY_BRIDGE,   { 0x063a, 0x063e } },
	{ "Haswell",      MA_INTEL_HASWELL,      { 0x063c, 0x0645, 0x0646, 0x063f } },
	{ "Broadwell",    MA_INTEL_BROADWELL,    { 0x063d, 0x0647, 0x0656 } },
	{ "Skylake",      MA_INTEL_SKYLAKE,      { 0x064e, 0x065e } },
//	{ "", MA_, { 0x0600 } },
};


#ifdef __i386__
const TUNEDFUNC_PREDICT tfnPredictI686 = {
	NULL,
	{ 0, 0 },
	cpp_PredictWrongMedianAndCount,
	cpp_PredictLeftAndCount,
	i686_RestoreWrongMedian,
	cpp_RestoreWrongMedianBlock4,
};
#endif

const TUNEDFUNC_PREDICT tfnPredictSSE2 = {
#ifdef __i386__
	&tfnPredictI686,
#else
	NULL,
#endif
	{ FEATURE0_SSE2, 0 },
	sse2_PredictWrongMedianAndCount,
	sse2_PredictLeftAndCount,
	sse1mmx_RestoreWrongMedian,
	sse2_RestoreWrongMedianBlock4,
	cpp_RestoreLeft,
	cpp_RestoreCylindricalLeft<10>,
};

const TUNEDFUNC_PREDICT tfnPredictSSSE3 = {
	&tfnPredictSSE2,
	{ FEATURE0_SSSE3, 0 },
	sse2_PredictWrongMedianAndCount,
	sse2_PredictLeftAndCount,
	sse1mmx_RestoreWrongMedian,
	sse2_RestoreWrongMedianBlock4,
	tuned_RestoreLeft8<CODEFEATURE_SSSE3>,
	tuned_RestoreLeft10<CODEFEATURE_SSSE3>,
};

const TUNEDFUNC_PREDICT tfnPredictAVX1 = {
	&tfnPredictSSSE3,
	{ FEATURE0_AVX1, 0 },
	sse2_PredictWrongMedianAndCount,
	sse2_PredictLeftAndCount,
	sse1mmx_RestoreWrongMedian,
	sse2_RestoreWrongMedianBlock4,
	tuned_RestoreLeft8<CODEFEATURE_AVX1>,
	tuned_RestoreLeft10<CODEFEATURE_AVX1>,
};


const TUNEDFUNC_HUFFMAN_ENCODE tfnHuffmanEncodeI686 = {
	NULL,
	{ 0, 0 },
	i686_HuffmanEncode,
};


const TUNEDFUNC_HUFFMAN_DECODE tfnHuffmanDecodeI686 = {
	NULL,
	{ 0, 0 },
	i686_HuffmanDecode,
};

#ifdef __x86_64__
const TUNEDFUNC_HUFFMAN_DECODE tfnHuffmanDecodeBMI2 = {
	&tfnHuffmanDecodeI686,
	{ 0, FEATURE1_BMI2 },
	bmi2_HuffmanDecode,
};
#endif


const TUNEDFUNC_CONVERT_YUVRGB tfnConvertYUVRGBSSE2 = {
	&tfnConvertYUVRGBCPP,
	{ FEATURE0_SSE2, 0 },
	{
		sse2_ConvertULY2ToBGR,
		sse2_ConvertULY2ToBGRX,
		sse2_ConvertULY2ToRGB,
		sse2_ConvertULY2ToXRGB,
		sse2_ConvertBGRToULY2,
		sse2_ConvertBGRXToULY2,
		sse2_ConvertRGBToULY2,
		sse2_ConvertXRGBToULY2,
		cpp_ConvertULY4ToRGB<CBT601Coefficient, CBGRColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_SSE2, CBT601Coefficient, CBGRAColorOrder>,
		cpp_ConvertULY4ToRGB<CBT601Coefficient, CRGBColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_SSE2, CBT601Coefficient, CARGBColorOrder>,
		cpp_ConvertRGBToULY4<CBT601Coefficient, CBGRColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_SSE2, CBT601Coefficient, CBGRAColorOrder>,
		cpp_ConvertRGBToULY4<CBT601Coefficient, CRGBColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_SSE2, CBT601Coefficient, CARGBColorOrder>,
	},
	{
		sse2_ConvertULH2ToBGR,
		sse2_ConvertULH2ToBGRX,
		sse2_ConvertULH2ToRGB,
		sse2_ConvertULH2ToXRGB,
		sse2_ConvertBGRToULH2,
		sse2_ConvertBGRXToULH2,
		sse2_ConvertRGBToULH2,
		sse2_ConvertXRGBToULH2,
		cpp_ConvertULY4ToRGB<CBT709Coefficient, CBGRColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_SSE2, CBT709Coefficient, CBGRAColorOrder>,
		cpp_ConvertULY4ToRGB<CBT709Coefficient, CRGBColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_SSE2, CBT709Coefficient, CARGBColorOrder>,
		cpp_ConvertRGBToULY4<CBT709Coefficient, CBGRColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_SSE2, CBT709Coefficient, CBGRAColorOrder>,
		cpp_ConvertRGBToULY4<CBT709Coefficient, CRGBColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_SSE2, CBT709Coefficient, CARGBColorOrder>,
	},
};

const TUNEDFUNC_CONVERT_YUVRGB tfnConvertYUVRGBSSSE3 = {
	&tfnConvertYUVRGBSSE2,
	{ FEATURE0_SSSE3, 0 },
	{
		sse2_ConvertULY2ToBGR,
		sse2_ConvertULY2ToBGRX,
		sse2_ConvertULY2ToRGB,
		sse2_ConvertULY2ToXRGB,
		ssse3_ConvertBGRToULY2,
		ssse3_ConvertBGRXToULY2,
		ssse3_ConvertRGBToULY2,
		ssse3_ConvertXRGBToULY2,
		tuned_ConvertULY4ToRGB<CODEFEATURE_SSSE3, CBT601Coefficient, CBGRColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_SSE2, CBT601Coefficient, CBGRAColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_SSSE3, CBT601Coefficient, CRGBColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_SSE2, CBT601Coefficient, CARGBColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_SSSE3, CBT601Coefficient, CBGRColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_SSSE3, CBT601Coefficient, CBGRAColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_SSSE3, CBT601Coefficient, CRGBColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_SSSE3, CBT601Coefficient, CARGBColorOrder>,
	},
	{
		sse2_ConvertULH2ToBGR,
		sse2_ConvertULH2ToBGRX,
		sse2_ConvertULH2ToRGB,
		sse2_ConvertULH2ToXRGB,
		ssse3_ConvertBGRToULH2,
		ssse3_ConvertBGRXToULH2,
		ssse3_ConvertRGBToULH2,
		ssse3_ConvertXRGBToULH2,
		tuned_ConvertULY4ToRGB<CODEFEATURE_SSSE3, CBT709Coefficient, CBGRColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_SSE2, CBT709Coefficient, CBGRAColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_SSSE3, CBT709Coefficient, CRGBColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_SSE2, CBT709Coefficient, CARGBColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_SSSE3, CBT709Coefficient, CBGRColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_SSSE3, CBT709Coefficient, CBGRAColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_SSSE3, CBT709Coefficient, CRGBColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_SSSE3, CBT709Coefficient, CARGBColorOrder>,
	},
};

const TUNEDFUNC_CONVERT_YUVRGB tfnConvertYUVRGBSSE41 = {
	&tfnConvertYUVRGBSSSE3,
	{ FEATURE0_SSE41, 0 },
	{
		sse41_ConvertULY2ToBGR,
		sse41_ConvertULY2ToBGRX,
		sse41_ConvertULY2ToRGB,
		sse41_ConvertULY2ToXRGB,
		ssse3_ConvertBGRToULY2,
		ssse3_ConvertBGRXToULY2,
		ssse3_ConvertRGBToULY2,
		ssse3_ConvertXRGBToULY2,
		tuned_ConvertULY4ToRGB<CODEFEATURE_SSSE3, CBT601Coefficient, CBGRColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_SSE2, CBT601Coefficient, CBGRAColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_SSSE3, CBT601Coefficient, CRGBColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_SSE2, CBT601Coefficient, CARGBColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_SSSE3, CBT601Coefficient, CBGRColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_SSSE3, CBT601Coefficient, CBGRAColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_SSSE3, CBT601Coefficient, CRGBColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_SSSE3, CBT601Coefficient, CARGBColorOrder>,
	},
	{
		sse41_ConvertULH2ToBGR,
		sse41_ConvertULH2ToBGRX,
		sse41_ConvertULH2ToRGB,
		sse41_ConvertULH2ToXRGB,
		ssse3_ConvertBGRToULH2,
		ssse3_ConvertBGRXToULH2,
		ssse3_ConvertRGBToULH2,
		ssse3_ConvertXRGBToULH2,
		tuned_ConvertULY4ToRGB<CODEFEATURE_SSSE3, CBT709Coefficient, CBGRColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_SSE2, CBT709Coefficient, CBGRAColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_SSSE3, CBT709Coefficient, CRGBColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_SSE2, CBT709Coefficient, CARGBColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_SSSE3, CBT709Coefficient, CBGRColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_SSSE3, CBT709Coefficient, CBGRAColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_SSSE3, CBT709Coefficient, CRGBColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_SSSE3, CBT709Coefficient, CARGBColorOrder>,
	},
};

const TUNEDFUNC_CONVERT_YUVRGB tfnConvertYUVRGBAVX1 = {
	&tfnConvertYUVRGBSSE41,
	{ FEATURE0_AVX1, 0 },
	{
		sse41_ConvertULY2ToBGR,
		sse41_ConvertULY2ToBGRX,
		sse41_ConvertULY2ToRGB,
		sse41_ConvertULY2ToXRGB,
		ssse3_ConvertBGRToULY2,
		ssse3_ConvertBGRXToULY2,
		ssse3_ConvertRGBToULY2,
		ssse3_ConvertXRGBToULY2,
		tuned_ConvertULY4ToRGB<CODEFEATURE_AVX1, CBT601Coefficient, CBGRColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_AVX1, CBT601Coefficient, CBGRAColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_AVX1, CBT601Coefficient, CRGBColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_AVX1, CBT601Coefficient, CARGBColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_AVX1, CBT601Coefficient, CBGRColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_AVX1, CBT601Coefficient, CBGRAColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_AVX1, CBT601Coefficient, CRGBColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_AVX1, CBT601Coefficient, CARGBColorOrder>,
	},
	{
		sse41_ConvertULH2ToBGR,
		sse41_ConvertULH2ToBGRX,
		sse41_ConvertULH2ToRGB,
		sse41_ConvertULH2ToXRGB,
		ssse3_ConvertBGRToULH2,
		ssse3_ConvertBGRXToULH2,
		ssse3_ConvertRGBToULH2,
		ssse3_ConvertXRGBToULH2,
		tuned_ConvertULY4ToRGB<CODEFEATURE_AVX1, CBT709Coefficient, CBGRColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_AVX1, CBT709Coefficient, CBGRAColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_AVX1, CBT709Coefficient, CRGBColorOrder>,
		tuned_ConvertULY4ToRGB<CODEFEATURE_AVX1, CBT709Coefficient, CARGBColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_AVX1, CBT709Coefficient, CBGRColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_AVX1, CBT709Coefficient, CBGRAColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_AVX1, CBT709Coefficient, CRGBColorOrder>,
		tuned_ConvertRGBToULY4<CODEFEATURE_AVX1, CBT709Coefficient, CARGBColorOrder>,
	},
};


const TUNEDFUNC_CONVERT_SHUFFLE tfnConvertShuffleSSSE3 = {
	&tfnConvertShuffleCPP,
	{ FEATURE0_SSSE3, 0 },
	ssse3_ConvertBGRToULRG,
	ssse3_ConvertBGRXToULRG,
	ssse3_ConvertXRGBToULRG,
	ssse3_ConvertBGRAToULRA,
	ssse3_ConvertARGBToULRA,
	ssse3_ConvertYUYVToULY2,
	ssse3_ConvertUYVYToULY2,
	ssse3_ConvertULRGToBGR,
	ssse3_ConvertULRGToBGRX,
	ssse3_ConvertULRGToXRGB,
	ssse3_ConvertULRAToBGRA,
	ssse3_ConvertULRAToARGB,
	ssse3_ConvertULY2ToYUYV,
	ssse3_ConvertULY2ToUYVY,
};


const TUNEDFUNC_CORRELATE tfnCorrelateSSSE3 = {
	NULL,
	{ FEATURE0_SSSE3, 0 },
	ssse3_EncorrelateInplaceBGRX,
	ssse3_EncorrelateInplaceBGRA,
	ssse3_EncorrelateInplaceXRGB,
	ssse3_EncorrelateInplaceARGB,
};


const TUNEDFUNC tfnRoot = {
	&tfnPredictAVX1,
	&tfnHuffmanEncodeI686,
#ifdef __x86_64__
	&tfnHuffmanDecodeBMI2,
#else
	&tfnHuffmanDecodeI686,
#endif
	&tfnConvertYUVRGBAVX1,
	&tfnConvertShuffleSSSE3,
	&tfnCorrelateSSSE3,
};

uint32_t dwSupportedFeatures[FEATURESIZE];


class CTunedFuncInitializer
{
	/*
	 * 環境変数が、
	 *   なければ false
	 *   空文字列なら false
	 *   "1" なら true
	 *   それ以外の場合は未定義
	 */
	static bool IsTunedFuncDisabled()
	{
		static const char szEnvName[] = "UTVIDEO_DISABLE_TUNED_FUNC";
#if defined(_WIN32)
		char p[4];
		auto n = GetEnvironmentVariableA(szEnvName, p, sizeof(p));
		if (n == 0)
			return false;
#endif
#if defined(__APPLE__) || defined(__unix__)
		char *p;
		p = getenv(szEnvName);
		if (p == NULL)
			return false;
#endif
		if (strcmp(p, "") == 0)
			return false;

		return true;
	}

public:
	CTunedFuncInitializer()
	{
		CLogInitializer::Initialize();

		if (IsTunedFuncDisabled())
			return;

		cpuid_result cpuid_0   = { 0, 0, 0, 0 };
		cpuid_result cpuid_1   = { 0, 0, 0, 0 };
		cpuid_result cpuid_7_0 = { 0, 0, 0, 0 };
		cpuid_result cpuid_ex0 = { 0, 0, 0, 0 };
		cpuid_result cpuid_ex1 = { 0, 0, 0, 0 };
		cpuid_result cpuid_ex2 = { 0, 0, 0, 0 };
		cpuid_result cpuid_ex3 = { 0, 0, 0, 0 };
		cpuid_result cpuid_ex4 = { 0, 0, 0, 0 };

		char vendor[16];
		char procbrand[64];
		unsigned int family, model;
		const char *maname;
		uint32_t ma;

		memset(dwSupportedFeatures, 0, sizeof(dwSupportedFeatures));

		cpuid(&cpuid_0, 0, 0);
		*(uint32_t *)(vendor+0) = cpuid_0.ebx;
		*(uint32_t *)(vendor+4) = cpuid_0.edx;
		*(uint32_t *)(vendor+8) = cpuid_0.ecx;
		vendor[12] = '\0';
		LOGPRINTF("CPUID     vendor=\"%s\"", vendor);

		cpuid(&cpuid_ex0, 0x80000000, 0);

		if (cpuid_ex0.eax >= 0x80000001)
		{
			cpuid(&cpuid_ex1, 0x80000001, 0);
		}

		if (cpuid_ex0.eax >= 0x80000004)
		{
			cpuid(&cpuid_ex2, 0x80000002, 0);
			cpuid(&cpuid_ex3, 0x80000003, 0);
			cpuid(&cpuid_ex4, 0x80000004, 0);
			*(uint32_t *)(procbrand+ 0) = cpuid_ex2.eax;
			*(uint32_t *)(procbrand+ 4) = cpuid_ex2.ebx;
			*(uint32_t *)(procbrand+ 8) = cpuid_ex2.ecx;
			*(uint32_t *)(procbrand+12) = cpuid_ex2.edx;
			*(uint32_t *)(procbrand+16) = cpuid_ex3.eax;
			*(uint32_t *)(procbrand+20) = cpuid_ex3.ebx;
			*(uint32_t *)(procbrand+24) = cpuid_ex3.ecx;
			*(uint32_t *)(procbrand+28) = cpuid_ex3.edx;
			*(uint32_t *)(procbrand+32) = cpuid_ex4.eax;
			*(uint32_t *)(procbrand+36) = cpuid_ex4.ebx;
			*(uint32_t *)(procbrand+40) = cpuid_ex4.ecx;
			*(uint32_t *)(procbrand+44) = cpuid_ex4.edx;
			procbrand[48] = '\0';
			LOGPRINTF("CPUID     procbrand=\"%s\"", procbrand);
		}

		if (cpuid_0.eax >= 1)
		{
			cpuid(&cpuid_1, 1, 0);
		}

		if (cpuid_0.eax >= 7)
		{
			cpuid(&cpuid_7_0, 7, 0);
		}


		family = ((cpuid_1.eax >> 20) & 0xff) + ((cpuid_1.eax >> 8) & 0xf);
		model  = ((cpuid_1.eax >> 12) & 0xf0) | ((cpuid_1.eax >> 4) & 0xf);
		LOGPRINTF("CPUID     family=%02XH model=%02XH", family, model);
		for (int i = 0; i < _countof(mafm); i++)
		{
			for (int j = 0; j < _countof(mafm[i].fm); j++)
			{
				if (mafm[i].fm[j] == ((family << 8) | model))
				{
					ma = mafm[i].ma;
					maname = mafm[i].name;
					goto ma_found;
				}
			}
		}
		maname = "Unknown";
		ma = MA_UNKNOWN;
ma_found:
		LOGPRINTF("CPUID     march=\"%s\" (ID %08X)", maname, ma);


		if (cpuid_7_0.ebx & (1 << 3))
		{
			LOGPRINTF("supports BMI1");
			dwSupportedFeatures[1] |= FEATURE1_BMI1;
		}

		if (cpuid_7_0.ebx & (1 << 8))
		{
			LOGPRINTF("supports BMI2");
			dwSupportedFeatures[1] |= FEATURE1_BMI2;
		}

		if (cpuid_ex1.ecx & (1 << 5))
		{
			LOGPRINTF("supports LZCNT");
			dwSupportedFeatures[1] |= FEATURE1_LZCNT;
		}

		if (cpuid_1.ecx & (1 << 23))
		{
			LOGPRINTF("supports POPCNT");
			dwSupportedFeatures[1] |= FEATURE1_POPCNT;
		}

		if (cpuid_1.ecx & (1 << 22))
		{
			LOGPRINTF("supports MOVBE");
			dwSupportedFeatures[1] |= FEATURE1_MOVBE;
		}


		if (cpuid_1.ecx & (1 << 27))
		{
			LOGPRINTF("supports OSXSAVE");

			xgetbv_result xgetbv_0 = { 0, 0 };

			xgetbv(&xgetbv_0, 0);

			if ((xgetbv_0.eax & 6) == 6)
			{
				LOGPRINTF("supports XMM/YMM state by OS");

				if (cpuid_7_0.ebx & (1 << 5))
				{
					LOGPRINTF("supports AVX2");
					dwSupportedFeatures[0] |= FEATURE0_AVX2;
				}

				if (cpuid_1.ecx & (1 << 28))
				{
					LOGPRINTF("supports AVX1");
					dwSupportedFeatures[0] |= FEATURE0_AVX1;
				}
			}
			else
			{
				LOGPRINTF("does not support XMM/YMM state by OS");
			}
		}
		else
		{
			LOGPRINTF("does not support OSXSAVE");
		}

		if (cpuid_1.ecx & (1 << 20))
		{
			LOGPRINTF("supports SSE4.2");
			dwSupportedFeatures[0] |= FEATURE0_SSE42;
		}

		if (cpuid_1.ecx & (1 << 19))
		{
			LOGPRINTF("supports SSE4.1");
			dwSupportedFeatures[0] |= FEATURE0_SSE41;
		}

		if (cpuid_1.ecx & (1 << 9))
		{
			LOGPRINTF("supports SSSE3");
			dwSupportedFeatures[0] |= FEATURE0_SSSE3;
		}

		if (cpuid_1.ecx & (1 << 0))
		{
			LOGPRINTF("supports SSE3");
			dwSupportedFeatures[0] |= FEATURE0_SSE3;
		}

		if (cpuid_1.edx & (1 << 26))
		{
			LOGPRINTF("supports SSE2");
			dwSupportedFeatures[0] |= FEATURE0_SSE2;
		}

		ResolveTunedFunc(&tfnRoot, dwSupportedFeatures);
	}
} tfi;
