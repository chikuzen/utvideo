/* �����R�[�h�͂r�i�h�r ���s�R�[�h�͂b�q�k�e */
/* $Id$ */

#pragma once

#ifdef _WIN32
DLLEXPORT int UtVideoFormatToWindowsFormat(DWORD *biCompression, WORD *biBitCount, GUID *subtype, utvf_t utvf);
DLLEXPORT int WindowsFormatToUtVideoFormat(utvf_t *utvf, DWORD biCompression, WORD biBitCount, REFGUID subtype);
DLLEXPORT int WindowsFormatToUtVideoFormat(utvf_t *utvf, DWORD biCompression, WORD biBitCount);
DLLEXPORT int WindowsFormatToUtVideoFormat(utvf_t *utvf, REFGUID subtype);
#endif

#if 0
DLLEXPORT int UtVideoFormatToQuickTimeFormat(OSType *pixelFormat, utvf_t utvf);
DLLEXPORT int QuickTimeFormatToUtVideoFormat(utvf_t *utvf, OSType pixelFormat);
#endif
