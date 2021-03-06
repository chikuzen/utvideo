/* 文字コードはＳＪＩＳ 改行コードはＣＲＬＦ */
/* $Id$ */

#include "stdafx.h"
#include "utvideo.h"
//#include <windows.h>
//#include <algorithm>
//using namespace std;
#include "HuffmanCode.h"
#include "TunedFunc.h"

struct hufftree {
	struct hufftree *left;
	struct hufftree *right;
	uint32_t count;
	uint32_t symbol;	// uint8_t ではないシンボルが入ることがある
};

inline bool hufftree_gt(const struct hufftree *a, const struct hufftree *b)
{
	return (a->count > b->count);
}

bool generate_code_length(uint8_t *codelen, const struct hufftree *node, uint8_t curlen)
{
	if (node->left == NULL) {
		codelen[node->symbol] = curlen;
		return (curlen > 24);
	} else {
		return
			generate_code_length(codelen, node->left, curlen+1) ||
			generate_code_length(codelen, node->right, curlen+1);
	}
}

static void GenerateLengthLimitedHuffmanCodeLengthTable(uint8_t *pCodeLengthTable, int nSymbolBits)
{
	// とりあえずこれで逃げる。
	memset(pCodeLengthTable, nSymbolBits, (size_t)1 << nSymbolBits);
}

template<int B>
void GenerateHuffmanCodeLengthTable(HUFFMAN_CODELEN_TABLE<B> *pCodeLengthTable, const uint32_t *pCountTable)
{
	struct hufftree *huffsort[1 << B];
	struct hufftree huffleaf[1 << B];
	struct hufftree huffnode[1 << B];
	int nsym;

	nsym = 0;
	for (int i = 0; i < (1 << B); i++) {
		if (pCountTable[i] != 0) {
			huffleaf[nsym].left = NULL;
			huffleaf[nsym].right = NULL;
			huffleaf[nsym].count = pCountTable[i];
			huffleaf[nsym].symbol = i;
			huffsort[nsym] = &huffleaf[nsym];
			nsym++;
		}
		else
			pCodeLengthTable->codelen[i] = 255;
	}

	std::sort(huffsort, huffsort+nsym, hufftree_gt);
	for (int i = nsym - 2; i >= 0; i--) {
		huffnode[i].left = huffsort[i];
		huffnode[i].right = huffsort[i+1];
		huffnode[i].count = huffsort[i]->count + huffsort[i+1]->count;

		struct hufftree **insptr = std::upper_bound(huffsort, huffsort+i, &huffnode[i], hufftree_gt);
		struct hufftree **movptr;
		for (movptr = huffsort+i-1; movptr >= insptr; movptr--)
			*(movptr+1) = *movptr;
		*insptr = &huffnode[i];
	}

	if (generate_code_length(pCodeLengthTable->codelen, huffsort[0], 0))
		GenerateLengthLimitedHuffmanCodeLengthTable(pCodeLengthTable->codelen, B);
}

template void GenerateHuffmanCodeLengthTable<8>(HUFFMAN_CODELEN_TABLE<8> *pCodeLengthTable, const uint32_t *pCountTable);
template void GenerateHuffmanCodeLengthTable<10>(HUFFMAN_CODELEN_TABLE<10> *pCodeLengthTable, const uint32_t *pCountTable);

template<int B>
bool cls_less(const SYMBOL_AND_CODELEN<B> &a, const SYMBOL_AND_CODELEN<B> &b)
{
	if (a.codelen != b.codelen)
		return a.codelen < b.codelen;
	else
		return a.symbol < b.symbol;
}

template<int B>
void GenerateHuffmanEncodeTable(HUFFMAN_ENCODE_TABLE<B> *pEncodeTable, const HUFFMAN_CODELEN_TABLE<B> *pCodeLengthTable)
{
	struct SYMBOL_AND_CODELEN<B> cls[1 << B];
	uintenc_t curcode;

	for (int i = 0; i < (1 << B); i++)
	{
		cls[i].symbol = i;
		cls[i].codelen = pCodeLengthTable->codelen[i];
	}

	std::sort(cls, cls + (1 << B), cls_less<B>);

	if (cls[0].codelen == 0)
	{
		memset(pEncodeTable, 0, sizeof(HUFFMAN_ENCODE_TABLE<B>));
		return;
	}

	memset(pEncodeTable, 0xff, sizeof(HUFFMAN_ENCODE_TABLE<B>));

	curcode = 0;
	for (int i = (1 << B) - 1; i >= 0; i--)
	{
		if (cls[i].codelen == 255)
			continue;
		pEncodeTable->dwTableMux[cls[i].symbol] = curcode | cls[i].codelen;
		curcode += UINTENC_MSB >> (cls[i].codelen - 1);
	}
}

template void GenerateHuffmanEncodeTable<8>(HUFFMAN_ENCODE_TABLE<8> *pEncodeTable, const HUFFMAN_CODELEN_TABLE<8> *pCodeLengthTable);
template void GenerateHuffmanEncodeTable<10>(HUFFMAN_ENCODE_TABLE<10> *pEncodeTable, const HUFFMAN_CODELEN_TABLE<10> *pCodeLengthTable);

// IA-32 の BSR 命令
// 本物の BSR 命令では入力が 0 の場合に出力が不定になる。
inline int bsr(uint32_t x)
{
	_ASSERT(x != 0);

	for (int i = 31; i >= 0; i--)
		if (x & (1 << i))
			return i;
	return rand() % 32;
}

// LZCNT あるいは CLZ と呼ばれる命令
inline int lzcnt(uint32_t x)
{
	for (int i = 31; i >= 0; i--)
		if (x & (1 << i))
			return 31 - i;
	return 32;
}

// 関数テンプレートの部分特殊化は言語仕様上できない（らしい）ので、メンバ関数を持つクラステンプレートの部分特殊化で代替する
template<int B, int syms>
struct GenerateMultiSpeedTable0
{
	static void f(HUFFMAN_DECODE_TABLE<B> *pDecodeTable, const HUFFMAN_CODELEN_TABLE<B> *pCodeLengthTable, const SYMBOL_AND_CODELEN<B> *cls, const int *clsidx, const uint32_t *codes, uint32_t prefix, int preflen);
};

/*
template<int B>
struct GenerateMultiSpeedTable0<B, sizeof(uint32_t) / sizeof(symbol_t<B>)>
みたいな書き方はできない
*/

template<>
void GenerateMultiSpeedTable0<8, 4>::f(HUFFMAN_DECODE_TABLE<8> *pDecodeTable, const HUFFMAN_CODELEN_TABLE<8> *pCodeLengthTable, const SYMBOL_AND_CODELEN<8> *cls, const int *clsidx, const uint32_t *codes, uint32_t prefix, int preflen) {}

template<>
void GenerateMultiSpeedTable0<10, 2>::f(HUFFMAN_DECODE_TABLE<10> *pDecodeTable, const HUFFMAN_CODELEN_TABLE<10> *pCodeLengthTable, const SYMBOL_AND_CODELEN<10> *cls, const int *clsidx, const uint32_t *codes, uint32_t prefix, int preflen) {}

template<int B, int syms>
void GenerateMultiSpeedTable0<B, syms>::f(HUFFMAN_DECODE_TABLE<B> *pDecodeTable, const HUFFMAN_CODELEN_TABLE<B> *pCodeLengthTable, const SYMBOL_AND_CODELEN<B> *cls, const int *clsidx, const uint32_t *codes, uint32_t prefix, int preflen)
{
	// pDecodeTable       結果
	// pCodeLengthTable   あれ使ってない？
	// cls                符号語長とシンボルへの配列（符号語長でソート）
	// codes              各シンボルに対応する符号語
	// prefix             この呼び出しで塗りつぶす範囲を表すプレフィックス
	// preflen            ↑の長さ（ビット数）
	for (int i = clsidx[HUFFMAN_DECODE_TABLE<B>::LOOKUP_BITS - preflen]; i >= 0; i--)
	{
		uint32_t newprefix = prefix | (codes[i] >> preflen);
		int newpreflen = preflen + cls[i].codelen;
		int idx = newprefix >> (32 - HUFFMAN_DECODE_TABLE<B>::LOOKUP_BITS);

		for (int j = 0; j < (1 << (HUFFMAN_DECODE_TABLE<B>::LOOKUP_BITS - newpreflen)); j++)
		{
			pDecodeTable->MultiSpeedTable_cs[idx + j].codelen = newpreflen;
			pDecodeTable->MultiSpeedTable_cs[idx + j].symlen = syms + 1;
			pDecodeTable->MultiSpeedTable_sym[idx + j].symbols8[syms] = cls[i].symbol;
		}
		GenerateMultiSpeedTable0<B, syms + 1>::f(pDecodeTable, pCodeLengthTable, cls, clsidx, codes, newprefix, newpreflen);
	}
}

template<int B>
void GenerateMultiSpeedTable(HUFFMAN_DECODE_TABLE<B> *pDecodeTable, const HUFFMAN_CODELEN_TABLE<B> *pCodeLengthTable, const SYMBOL_AND_CODELEN<B> *cls, int nLastIndex)
{
	uint32_t codes[1 << B];
	uint32_t curcode;
	int clsidx[HUFFMAN_DECODE_TABLE<B>::LOOKUP_BITS + 1];
	int previdx;

	// 各シンボルに対応する符号語を計算する
	curcode = 0;
	for (int i = nLastIndex; i >= 0; i--)
	{
		codes[i] = curcode;
		curcode += 0x80000000 >> (cls[i].codelen - 1);
	}

	// 各符号語長に対し、その符号語長を持つもっともインデックスが大きい符号語を求める
	previdx = nLastIndex;
	for (int i = HUFFMAN_DECODE_TABLE<B>::LOOKUP_BITS; i >= 0; i--)
	{
		int j;
		for (j = previdx; j >= 0; j--)
		{
			if (cls[j].codelen <= i)
				break;
		}
		clsidx[i] = j;
		previdx = j;
	}

	// 結果テーブルの初期化
	for (int i = 0; i < _countof(pDecodeTable->MultiSpeedTable_cs); i++)
	{
		pDecodeTable->MultiSpeedTable_cs[i].codelen = 255;
		pDecodeTable->MultiSpeedTable_cs[i].symlen = 0;
		pDecodeTable->MultiSpeedTable_sym[i].symbols32 = 0;
	}

	GenerateMultiSpeedTable0<B, 0>::f(pDecodeTable, pCodeLengthTable, cls, clsidx, codes, 0, 0);
}

template<int B>
void GenerateHuffmanDecodeTable(HUFFMAN_DECODE_TABLE<B> *pDecodeTable, const HUFFMAN_CODELEN_TABLE<B> *pCodeLengthTable)
{
	struct SYMBOL_AND_CODELEN<B> cls[1 << B];
	int nLastIndex;

	for (int i = 0; i < (1 << B); i++)
	{
		cls[i].symbol = i;
		cls[i].codelen = pCodeLengthTable->codelen[i];
	}

	std::sort(cls, cls + (1 << B), cls_less<B>);

	// 出現するシンボルが１種類しかない場合の処理
	if (cls[0].codelen == 0)
	{
		pDecodeTable->MultiSpeedTable_cs[0].codelen = 0;
		pDecodeTable->MultiSpeedTable_sym[0].symbols8[0] = cls[0].symbol;
		return;
	}

	// 最も長い符号長を持つシンボルの cls 上での位置を求める
	for (nLastIndex = (1 << B) - 1; nLastIndex >= 0; nLastIndex--)
	{
		if (cls[nLastIndex].codelen != 255)
			break;
	}

	// 低速テーブルの生成
	{
		uint32_t curcode = 1; // bsr 対策で 0 ではなく 1。符号語長は 24 以下なので 1 にしてあっても問題ない。
		int j = 0;
		int base = 0;
		int nextfillidx = 0;
		int prevbsrval = 0;

		for (int i = nLastIndex; i >= 0; i--)
		{
			// 短い符号語の場合は高速テーブルでデコードされるので、低速テーブルの生成は不要である。
			if (cls[i].codelen <= HUFFMAN_DECODE_TABLE<B>::LOOKUP_BITS)
				break;

			int bsrval = bsr(curcode);
			if (bsrval != prevbsrval)
			{
				base = nextfillidx - (curcode >> (32 - cls[i].codelen));
			}
			for (; j <= bsrval; j++)
			{
				pDecodeTable->nCodeShift[j] = 32 - cls[i].codelen;
				pDecodeTable->dwSymbolBase[j] = base;
			}
			int lastfillidx = nextfillidx + (1 << (32 - pDecodeTable->nCodeShift[bsrval] - cls[i].codelen));
			for (; nextfillidx < lastfillidx; nextfillidx++)
			{
				pDecodeTable->SymbolAndCodeLength[nextfillidx].symbol  = cls[i].symbol;
				pDecodeTable->SymbolAndCodeLength[nextfillidx].codelen = cls[i].codelen;
			}
			curcode += 0x80000000 >> (cls[i].codelen - 1);
			prevbsrval = bsrval;
		}
	}

	GenerateMultiSpeedTable(pDecodeTable, pCodeLengthTable, cls, nLastIndex);
}

template void GenerateHuffmanDecodeTable<8>(HUFFMAN_DECODE_TABLE<8> *pDecodeTable, const HUFFMAN_CODELEN_TABLE<8> *pCodeLengthTable);
template void GenerateHuffmanDecodeTable<10>(HUFFMAN_DECODE_TABLE<10> *pDecodeTable, const HUFFMAN_CODELEN_TABLE<10> *pCodeLengthTable);

inline void FlushEncoded(uint32_t *&pDst, uintenc_t &dwTmpEncoded, int &nBits)
{
#if defined(__x86_64__)
	if (nBits > 0)
		*pDst++ = (uint32_t)(dwTmpEncoded >> 32);
	if (nBits > 32)
		*pDst++ = (uint32_t)(dwTmpEncoded & 0xffffffff);
#else
	if (nBits > 0)
		*pDst++ = dwTmpEncoded;
#endif
}

template<int B>
size_t cpp_HuffmanEncode(uint8_t *pDstBegin, const symbol_t<B> *pSrcBegin, const symbol_t<B> *pSrcEnd, const HUFFMAN_ENCODE_TABLE<B> *pEncodeTable)
{
	int nBits;
	uintenc_t dwTmpEncoded;
	uint32_t *pDst;
	const symbol_t<B> *p;

	if (pEncodeTable->dwTableMux[0] == 0)
		return 0;

	nBits = 0;
	dwTmpEncoded = 0;
	pDst = (uint32_t *)pDstBegin;

	for (p = pSrcBegin; p < pSrcEnd; p++)
	{
		_ASSERT(*p <= CSymbolBits<B>::maxval);

		int nCurBits = (int)(pEncodeTable->dwTableMux[*p] & 0xff);
		_ASSERT(nCurBits > 0 && nCurBits != 0xff);
		uintenc_t dwCurEncoded = pEncodeTable->dwTableMux[*p] & UINTENC_MASK;

		dwTmpEncoded |= dwCurEncoded >> nBits;
		nBits += nCurBits;
		if (nBits >= UINTENC_BITS)
		{
			FlushEncoded(pDst, dwTmpEncoded, nBits);
			nBits -= UINTENC_BITS;
			dwTmpEncoded = dwCurEncoded << (nCurBits - nBits);
		}
	}

	FlushEncoded(pDst, dwTmpEncoded, nBits);

	return ((uint8_t *)pDst) - pDstBegin;
}

template size_t cpp_HuffmanEncode<8>(uint8_t *pDstBegin, const CSymbolBits<8>::symbol_t *pSrcBegin, const CSymbolBits<8>::symbol_t *pSrcEnd, const HUFFMAN_ENCODE_TABLE<8> *pEncodeTable);
template size_t cpp_HuffmanEncode<10>(uint8_t *pDstBegin, const CSymbolBits<10>::symbol_t *pSrcBegin, const CSymbolBits<10>::symbol_t *pSrcEnd, const HUFFMAN_ENCODE_TABLE<10> *pEncodeTable);


template<int B>
symbol_t<B> * cpp_HuffmanDecode(symbol_t<B> *pDstBegin, symbol_t<B> *pDstEnd, const uint8_t *pSrcBegin, const HUFFMAN_DECODE_TABLE<B> *pDecodeTable)
{
	int nBits;
	uint32_t *pSrc;
	symbol_t<B> *pDst;

	nBits = 0;
	pSrc = (uint32_t *)pSrcBegin;

	for (pDst = (symbol_t<B> *)pDstBegin; pDst < (symbol_t<B> *)pDstEnd;)
	{
		uint32_t code;
		uint32_t symbol32;
		int codelen;
		int symlen;

		if (nBits == 0)
			code = (*pSrc) | 0x00000001;
		else
			code = ((*pSrc) << nBits) | ((*(pSrc + 1)) >> (32 - nBits)) | 0x00000001;

		int tableidx = code >> (32 - HUFFMAN_DECODE_TABLE<B>::LOOKUP_BITS);
		if (pDecodeTable->MultiSpeedTable_cs[tableidx].codelen != 255)
		{
			symbol32 = pDecodeTable->MultiSpeedTable_sym[tableidx].symbols32;
			codelen = pDecodeTable->MultiSpeedTable_cs[tableidx].codelen;
			symlen = pDecodeTable->MultiSpeedTable_cs[tableidx].symlen;
		}
		else
		{
			int bsrval = bsr(code);
			int codeshift = pDecodeTable->nCodeShift[bsrval];
			code >>= codeshift;
			tableidx = pDecodeTable->dwSymbolBase[bsrval] + code;
			symbol32 = pDecodeTable->SymbolAndCodeLength[tableidx].symbol;
			codelen = pDecodeTable->SymbolAndCodeLength[tableidx].codelen;
			symlen = 1;
		}

		for (int i = 0; i < symlen && pDst < (symbol_t<B> *)pDstEnd; i++)
		{
			*pDst++ = symbol32;
			symbol32 >>= sizeof(symbol_t<B>) * 8;
		}
		nBits += codelen;

		if (nBits >= 32)
		{
			nBits -= 32;
			pSrc++;
		}
	}

	return pDst;
}

template symbol_t<8> * cpp_HuffmanDecode<8>(symbol_t<8> *pDstBegin, symbol_t<8> *pDstEnd, const uint8_t *pSrcBegin, const HUFFMAN_DECODE_TABLE<8> *pDecodeTable);
template symbol_t<10> * cpp_HuffmanDecode<10>(symbol_t<10> *pDstBegin, symbol_t<10> *pDstEnd, const uint8_t *pSrcBegin, const HUFFMAN_DECODE_TABLE<10> *pDecodeTable);


template<int B>
struct tfnHuffmanDecode
{
	static symbol_t<B> *f(symbol_t<B> *pDstBegin, symbol_t<B> *pDstEnd, const uint8_t *pSrcBegin, const HUFFMAN_DECODE_TABLE<B> *pDecodeTable);
};

template<>
struct tfnHuffmanDecode<8>
{
	static symbol_t<8> *f(symbol_t<8> *pDstBegin, symbol_t<8> *pDstEnd, const uint8_t *pSrcBegin, const HUFFMAN_DECODE_TABLE<8> *pDecodeTable)
	{
		return HuffmanDecode8(pDstBegin, pDstEnd, pSrcBegin, pDecodeTable);
	}
};

template<>
struct tfnHuffmanDecode<10>
{
	static symbol_t<10> *f(symbol_t<10> *pDstBegin, symbol_t<10> *pDstEnd, const uint8_t *pSrcBegin, const HUFFMAN_DECODE_TABLE<10> *pDecodeTable)
	{
		return cpp_HuffmanDecode<10>(pDstBegin, pDstEnd, pSrcBegin, pDecodeTable);
	}
};

template<int B>
symbol_t<B> *HuffmanDecode(symbol_t<B> *pDstBegin, symbol_t<B> *pDstEnd, const uint8_t *pSrcBegin, const HUFFMAN_DECODE_TABLE<B> *pDecodeTable)
{
	if (pDecodeTable->MultiSpeedTable_cs[0].codelen == 0)
	{
		std::fill(pDstBegin, pDstEnd, pDecodeTable->MultiSpeedTable_sym[0].symbols8[0]);
		return pDstEnd;
	}
	else
		return tfnHuffmanDecode<B>::f(pDstBegin, pDstEnd, pSrcBegin, pDecodeTable);
}

template symbol_t<8> *HuffmanDecode<8>(symbol_t<8> *pDstBegin, symbol_t<8> *pDstEnd, const uint8_t *pSrcBegin, const HUFFMAN_DECODE_TABLE<8> *pDecodeTable);
template symbol_t<10> *HuffmanDecode<10>(symbol_t<10> *pDstBegin, symbol_t<10> *pDstEnd, const uint8_t *pSrcBegin, const HUFFMAN_DECODE_TABLE<10> *pDecodeTable);
