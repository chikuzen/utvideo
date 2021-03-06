/* �����R�[�h�͂r�i�h�r ���s�R�[�h�͂b�q�k�e */
/* $Id$ */

#pragma once

#include "SymbolBits.h"

#if defined(__x86_64__)
typedef uint64_t uintenc_t;
#define UINTENC_BITS 64
#define UINTENC_MSB  0x8000000000000000ULL
#define UINTENC_MASK 0xffffffffffffff00ULL
#else
typedef uint32_t uintenc_t;
#define UINTENC_BITS 32
#define UINTENC_MSB  0x80000000U
#define UINTENC_MASK 0xffffff00U
#endif

template<int B>
struct SYMBOL_AND_CODELEN
{
	symbol_t<B> symbol;
	uint8_t codelen;
};

template<int B>
struct HUFFMAN_CODELEN_TABLE
{
	uint8_t codelen[1 << B];
};

template<int B>
struct HUFFMAN_ENCODE_TABLE
{
	uintenc_t dwTableMux[1 << B];
};

template<int B>
struct HUFFMAN_DECODE_TABLE
{
	static const int LOOKUP_BITS = (B == 8) ? 12 : 11; /* XXX */

	struct
	{
		uint8_t symlen;
		uint8_t codelen;
	} MultiSpeedTable_cs[1 << LOOKUP_BITS];
	union
	{
		uint32_t symbols32;
		symbol_t<B> symbols8[sizeof(uint32_t) / sizeof(symbol_t<B>)];
	} MultiSpeedTable_sym[1 << LOOKUP_BITS];

	uint8_t nCodeShift[32];
	uint32_t dwSymbolBase[32];
	SYMBOL_AND_CODELEN<B> SymbolAndCodeLength[(B == 8) ? 1024 : 4096*4 /* XXX */];
};

template<int B> void GenerateHuffmanCodeLengthTable(HUFFMAN_CODELEN_TABLE<B> *pCodeLengthTable, const uint32_t *pCountTable);
template<int B> void GenerateHuffmanEncodeTable(HUFFMAN_ENCODE_TABLE<B> *pEncodeTable, const HUFFMAN_CODELEN_TABLE<B> *pCodeLengthTable);
template<int B> void GenerateHuffmanDecodeTable(HUFFMAN_DECODE_TABLE<B> *pDecodeTable, const HUFFMAN_CODELEN_TABLE<B> *pCodeLengthTable);

template<int B> size_t cpp_HuffmanEncode(uint8_t *pDstBegin, const symbol_t<B> *pSrcBegin, const symbol_t<B> *pSrcEnd, const HUFFMAN_ENCODE_TABLE<B> *pEncodeTable);
template<int B> symbol_t<B> *cpp_HuffmanDecode(symbol_t<B> *pDstBegin, symbol_t<B> *pDstEnd, const uint8_t *pSrcBegin, const HUFFMAN_DECODE_TABLE<B> *pDecodeTable);

#if defined(__i386__) || defined(__x86_64__)
extern "C" size_t i686_HuffmanEncode(uint8_t *pDstBegin, const uint8_t *pSrcBegin, const uint8_t *pSrcEnd, const HUFFMAN_ENCODE_TABLE<8> *pEncodeTable);
extern "C" uint8_t *i686_HuffmanDecode(uint8_t *pDstBegin, uint8_t *pDstEnd, const uint8_t *pSrcBegin, const HUFFMAN_DECODE_TABLE<8> *pDecodeTable);

#if defined(__x86_64__)
extern "C" uint8_t *bmi2_HuffmanDecode(uint8_t *pDstBegin, uint8_t *pDstEnd, const uint8_t *pSrcBegin, const HUFFMAN_DECODE_TABLE<8> *pDecodeTable);
#endif
#endif

template<int B> symbol_t<B> *HuffmanDecode(symbol_t<B> *pDstBegin, symbol_t<B> *pDstEnd, const uint8_t *pSrcBegin, const HUFFMAN_DECODE_TABLE<B> *pDecodeTable);
