/* 文字コードはＳＪＩＳ 改行コードはＣＲＬＦ */
/* $Id$ */
/*
 * Ut Video Codec Suite
 * Copyright (C) 2008-2009  UMEZAWA Takeshi
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * 
 * 
 * Ut Video Codec Suite
 * Copyright (C) 2008-2009  梅澤 威志
 * 
 * このプログラムはフリーソフトウェアです。あなたはこれを、フリーソフ
 * トウェア財団によって発行された GNU 一般公衆利用許諾契約書(バージョ
 * ン2か、希望によってはそれ以降のバージョンのうちどれか)の定める条件
 * の下で再頒布または改変することができます。
 * 
 * このプログラムは有用であることを願って頒布されますが、*全くの無保
 * 証* です。商業可能性の保証や特定の目的への適合性は、言外に示された
 * ものも含め全く存在しません。詳しくはGNU 一般公衆利用許諾契約書をご
 * 覧ください。
 * 
 * あなたはこのプログラムと共に、GNU 一般公衆利用許諾契約書の複製物を
 * 一部受け取ったはずです。もし受け取っていなければ、フリーソフトウェ
 * ア財団まで請求してください(宛先は the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA)。
 */

#include "StdAfx.h"
#include "utvideo.h"
#include "VCMCodec.h"
#include "ULY2Encoder.h"
#include "ULY2Decoder.h"
#include "ULY0Encoder.h"
#include "ULY0Decoder.h"
#include "ULRGEncoder.h"
#include "ULRGDecoder.h"
#include "ULRAEncoder.h"
#include "ULRADecoder.h"

const CVCMCodec::CODECLIST CVCMCodec::m_codeclist[] = {
	{ -1,          "",       CDummyEncoder::CreateInstance, CDummyDecoder::CreateInstance },
	{ FCC('ULY2'), "YUV422", CULY2Encoder::CreateInstance,  CULY2Decoder::CreateInstance  },
	{ FCC('ULY0'), "YUV420", CULY0Encoder::CreateInstance,  CULY0Decoder::CreateInstance  },
	{ FCC('ULRG'), "RGB",    CULRGEncoder::CreateInstance,  CULRGDecoder::CreateInstance  },
	{ FCC('ULRA'), "RGBA",   CULRAEncoder::CreateInstance,  CULRADecoder::CreateInstance  },
};

void CVCMCodec::ICInstallAll(void)
{
	char szLongFilename[MAX_PATH];
	char szShortFilename[MAX_PATH];

	/*
	 * 何故か長いファイル名ではダメ（登録はできるがロードに失敗する）で、
	 * 8.3 形式のファイル名で登録しなければならない。
	 */
	GetModuleFileName(hModule, szLongFilename, sizeof(szLongFilename));
	GetShortPathName(szLongFilename, szShortFilename, sizeof(szShortFilename));
	for (int i = 1; i < _countof(m_codeclist); i++)
		ICInstall(ICTYPE_VIDEO, m_codeclist[i].fcc, (LPARAM)szShortFilename, NULL, ICINSTALL_DRIVER);
}

void CVCMCodec::ICRemoveAll(void)
{
	for (int i = 1; i < _countof(m_codeclist); i++)
		ICRemove(ICTYPE_VIDEO, m_codeclist[i].fcc, 0);
}

CVCMCodec::CVCMCodec(DWORD fccHandler)
{
	int idx;

	DEBUG_ENTER_LEAVE("CVCMCodec::CVCMCodec(DWORD) this=%p fccHandler=%08X (%c%c%c%c)", this, fccHandler, FCC4PRINTF(fccHandler));

	for (idx = 0; idx < _countof(m_codeclist); idx++)
	{
		if (m_codeclist[idx].fcc == fccHandler)
			break;
	}
	if (idx == _countof(m_codeclist))
		idx = 0;

	m_fccHandler         = m_codeclist[idx].fcc;
	m_pszColorFormatName = m_codeclist[idx].pszColorFormatName;
	m_pEncoder           = m_codeclist[idx].pfnCreateEncoder();
	m_pDecoder           = m_codeclist[idx].pfnCreateDecoder();
	_RPT2(_CRT_WARN, "infcc=%08X foundfcc=%08X\n", fccHandler, m_fccHandler);
}

CVCMCodec::~CVCMCodec(void)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::~CVCMCodec(void) this=%p", this);

	delete m_pEncoder;
	delete m_pDecoder;
}

CVCMCodec *CVCMCodec::Open(ICOPEN *icopen)
{
	union
	{
		DWORD fccHandler;
		char fccChar[4];
	};

	if (icopen != NULL)
	{
		if (icopen->fccType != ICTYPE_VIDEO)
			return NULL;
		fccHandler = icopen->fccHandler;
		// なぜか小文字で渡されることがあるので、最初に大文字化しておく。
		for (int i = 0; i < 4; i++)
			fccChar[i] = toupper(fccChar[i]);
		icopen->dwError = ICERR_OK;
	}
	else
		fccHandler = (DWORD)-1;

	return new CVCMCodec(fccHandler);
}

LRESULT CVCMCodec::QueryAbout(void)
{
	return ICERR_OK;
}

LRESULT CVCMCodec::About(HWND hwnd)
{
	char buf[256];

	wsprintf(buf, "Ut Video Codec Suite, Version %s (%s) \nCopyright (C) 2008-2009  UMEZAWA Takeshi",
		UTVIDEO_VERSION_STR,
		UTVIDEO_IMPLEMENTATION_STR);
	MessageBox(hwnd, buf, "Ut Video Codec Suite", MB_OK);
	return ICERR_OK;
}

LRESULT CVCMCodec::GetInfo(ICINFO *icinfo, SIZE_T cb)
{
	if (icinfo == NULL)
		return sizeof(ICINFO);

	if (cb < sizeof(ICINFO))
		return 0;

	icinfo->dwSize       = sizeof(ICINFO);
	icinfo->fccType      = ICTYPE_VIDEO;
	icinfo->fccHandler   = m_fccHandler;
	icinfo->dwFlags      = 0;
	icinfo->dwVersion    = UTVIDEO_VERSION_AND_IMPLEMENTATION;
	icinfo->dwVersionICM = ICVERSION;
	wsprintfW(icinfo->szName, L"Ut Video (%C%C%C%C)", FCC4PRINTF(m_fccHandler));
	wsprintfW(icinfo->szDescription, L"Ut Video Codec %S (%C%C%C%C) VCM (%S)",
		m_pszColorFormatName,
		FCC4PRINTF(m_fccHandler),
		UTVIDEO_IMPLEMENTATION_STR);

	return sizeof(ICINFO);
}

LRESULT CVCMCodec::QueryConfigure(void)
{
	return ICERR_OK;
}

LRESULT CVCMCodec::Configure(HWND hwnd)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::Configure() this=%p hwnd=%08X", this, hwnd);
	return m_pEncoder->Configure(hwnd);
}

LRESULT CVCMCodec::GetStateSize(void)
{
	return sizeof(ENCODERCONF);
}

LRESULT CVCMCodec::GetState(void *pState, SIZE_T cb)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::GetState() this=%p pState=%p, cb=%z", this, pState, cb);
	return m_pEncoder->GetState(pState, cb);
}

LRESULT CVCMCodec::SetState(const void *pState, SIZE_T cb)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::SetState() this=%p pState=%p, cb=%z", this, pState, cb);
	return m_pEncoder->SetState(pState, cb);
}

LRESULT CVCMCodec::Compress(const ICCOMPRESS *icc, SIZE_T cb)
{
	return m_pEncoder->Compress(icc, cb);
}

LRESULT CVCMCodec::CompressBegin(const BITMAPINFOHEADER *pbihIn, const BITMAPINFOHEADER *pbihOut)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::CompressBegin() this=%p", this);
	return m_pEncoder->CompressBegin(pbihIn, pbihOut);
}

LRESULT CVCMCodec::CompressEnd(void)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::CompressEnd() this=%p", this);
	return m_pEncoder->CompressEnd();
}

LRESULT CVCMCodec::CompressGetFormat(const BITMAPINFOHEADER *pbihIn, BITMAPINFOHEADER *pbihOut)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::CompressGetFormat() this=%p", this);
	return m_pEncoder->CompressGetFormat(pbihIn, pbihOut);
}

LRESULT CVCMCodec::CompressGetSize(const BITMAPINFOHEADER *pbihIn, const BITMAPINFOHEADER *pbihOut)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::CompressGetSize() this=%p", this);
	return m_pEncoder->CompressGetSize(pbihIn, pbihOut);
}

LRESULT CVCMCodec::CompressQuery(const BITMAPINFOHEADER *pbihIn, const BITMAPINFOHEADER *pbihOut)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::CompressQuery() this=%p", this);
	return m_pEncoder->CompressQuery(pbihIn, pbihOut);
}

LRESULT CVCMCodec::Decompress(const ICDECOMPRESS *icd, SIZE_T cb)
{
	return m_pDecoder->Decompress(icd, cb);
}

LRESULT CVCMCodec::DecompressBegin(const BITMAPINFOHEADER *pbihIn, const BITMAPINFOHEADER *pbihOut)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::DecompressBegin() this=%p", this);
	return m_pDecoder->DecompressBegin(pbihIn, pbihOut);
}

LRESULT CVCMCodec::DecompressEnd(void)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::DecompressEnd() this=%p", this);
	return m_pDecoder->DecompressEnd();
}

LRESULT CVCMCodec::DecompressGetFormat(const BITMAPINFOHEADER *pbihIn, BITMAPINFOHEADER *pbihOut)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::DecompressGetFormat() this=%p", this);
	return m_pDecoder->DecompressGetFormat(pbihIn, pbihOut);
}

LRESULT CVCMCodec::DecompressQuery(const BITMAPINFOHEADER *pbihIn, const BITMAPINFOHEADER *pbihOut)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::DecompressQuery() this=%p", this);
	return m_pDecoder->DecompressQuery(pbihIn, pbihOut);
}
