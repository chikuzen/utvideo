/* 文字コードはＳＪＩＳ 改行コードはＣＲＬＦ */
/* $Id$ */

#include "StdAfx.h"
#include "utvideo.h"
#include "VCMCodec.h"
#include "ULY2Codec.h"
#include "ULY0Codec.h"
#include "ULRGCodec.h"
#include "ULRACodec.h"

const CVCMCodec::CODECLIST CVCMCodec::m_codeclist[] = {
	{ -1,          "",       CDummyCodec::CreateInstance },
	{ FCC('ULY2'), "YUV422", CULY2Codec::CreateInstance  },
	{ FCC('ULY0'), "YUV420", CULY0Codec::CreateInstance  },
	{ FCC('ULRG'), "RGB",    CULRGCodec::CreateInstance  },
	{ FCC('ULRA'), "RGBA",   CULRACodec::CreateInstance  },
};

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
	m_pCodec             = m_codeclist[idx].pfnCreateCodec();
	_RPT2(_CRT_WARN, "infcc=%08X foundfcc=%08X\n", fccHandler, m_fccHandler);
}

CVCMCodec::~CVCMCodec(void)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::~CVCMCodec(void) this=%p", this);

	delete m_pCodec;
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

	wsprintf(buf, "Ut Video Codec Suite, Version %s %s \nCopyright (C) 2008-2010  UMEZAWA Takeshi",
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
	wsprintfW(icinfo->szDescription, L"Ut Video Codec %S (%C%C%C%C) %S",
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
	return m_pCodec->Configure(hwnd);
}

LRESULT CVCMCodec::GetStateSize(void)
{
	return sizeof(ENCODERCONF);
}

LRESULT CVCMCodec::GetState(void *pState, SIZE_T cb)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::GetState() this=%p pState=%p, cb=%z", this, pState, cb);
	return m_pCodec->GetState(pState, cb);
}

LRESULT CVCMCodec::SetState(const void *pState, SIZE_T cb)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::SetState() this=%p pState=%p, cb=%z", this, pState, cb);
	return m_pCodec->SetState(pState, cb);
}

LRESULT CVCMCodec::Compress(const ICCOMPRESS *icc, SIZE_T cb)
{
	return m_pCodec->Compress(icc, cb);
}

LRESULT CVCMCodec::CompressBegin(const BITMAPINFOHEADER *pbihIn, const BITMAPINFOHEADER *pbihOut)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::CompressBegin() this=%p", this);
	return m_pCodec->CompressBegin(pbihIn, pbihOut);
}

LRESULT CVCMCodec::CompressEnd(void)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::CompressEnd() this=%p", this);
	return m_pCodec->CompressEnd();
}

LRESULT CVCMCodec::CompressGetFormat(const BITMAPINFOHEADER *pbihIn, BITMAPINFOHEADER *pbihOut)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::CompressGetFormat() this=%p", this);
	return m_pCodec->CompressGetFormat(pbihIn, pbihOut);
}

LRESULT CVCMCodec::CompressGetSize(const BITMAPINFOHEADER *pbihIn, const BITMAPINFOHEADER *pbihOut)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::CompressGetSize() this=%p", this);
	return m_pCodec->CompressGetSize(pbihIn, pbihOut);
}

LRESULT CVCMCodec::CompressQuery(const BITMAPINFOHEADER *pbihIn, const BITMAPINFOHEADER *pbihOut)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::CompressQuery() this=%p", this);
	return m_pCodec->CompressQuery(pbihIn, pbihOut);
}

LRESULT CVCMCodec::Decompress(const ICDECOMPRESS *icd, SIZE_T cb)
{
	return m_pCodec->Decompress(icd, cb);
}

LRESULT CVCMCodec::DecompressBegin(const BITMAPINFOHEADER *pbihIn, const BITMAPINFOHEADER *pbihOut)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::DecompressBegin() this=%p", this);
	return m_pCodec->DecompressBegin(pbihIn, pbihOut);
}

LRESULT CVCMCodec::DecompressEnd(void)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::DecompressEnd() this=%p", this);
	return m_pCodec->DecompressEnd();
}

LRESULT CVCMCodec::DecompressGetFormat(const BITMAPINFOHEADER *pbihIn, BITMAPINFOHEADER *pbihOut)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::DecompressGetFormat() this=%p", this);
	return m_pCodec->DecompressGetFormat(pbihIn, pbihOut);
}

LRESULT CVCMCodec::DecompressQuery(const BITMAPINFOHEADER *pbihIn, const BITMAPINFOHEADER *pbihOut)
{
	DEBUG_ENTER_LEAVE("CVCMCodec::DecompressQuery() this=%p", this);
	return m_pCodec->DecompressQuery(pbihIn, pbihOut);
}
