/*
 * $Id: iofile.c 1.5 1996/08/05 18:51:19 chasan released $
 *
 * Input/Output file stream routines.
 *
 * Copyright (c) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <limits.h>
#include <malloc.h>
#include <string.h>
#include "audio.h"
#include "iofile.h"

/*
 * Audio I/O file stream routines
 */
static FILE *lpStream;

UINT AIAPI AIOOpenFile(LPSTR lpszFileName)
{
    if ((lpStream = fopen(lpszFileName, "rb")) != NULL)
        return AUDIO_ERROR_NONE;
    return AUDIO_ERROR_FILENOTFOUND;
}

UINT AIAPI AIOCloseFile(VOID)
{
    fclose(lpStream);
    return AUDIO_ERROR_NONE;
}

UINT AIAPI AIOSeekFile(LONG dwOffset, UINT nWhere)
{
    fseek(lpStream, dwOffset, nWhere);
    return AUDIO_ERROR_NONE;
}

UINT AIAPI AIOReadFile(LPVOID lpData, UINT nSize)
{
    if (fread(lpData, 1, nSize, lpStream) != nSize)
        memset(lpData, 0, nSize);
    return AUDIO_ERROR_NONE;
}

/* little-endian input routines */
UINT AIAPI AIOReadChar(LPBYTE lpData)
{
    if (fread(lpData, 1, sizeof(BYTE), lpStream) != sizeof(BYTE))
        memset(lpData, 0, sizeof(BYTE));
    return AUDIO_ERROR_NONE;
}

UINT AIAPI AIOReadShort(LPWORD lpData)
{
    if (fread(lpData, 1, sizeof(WORD), lpStream) != sizeof(WORD))
        memset(lpData, 0, sizeof(WORD));
#ifdef __BIGENDIAN__
    *lpData = MAKEWORD(HIBYTE(*lpData), LOBYTE(*lpData));
#endif
    return AUDIO_ERROR_NONE;
}

UINT AIAPI AIOReadLong(LPDWORD lpData)
{
    if (fread(lpData, 1, sizeof(DWORD), lpStream) != sizeof(DWORD))
        memset(lpData, 0, sizeof(DWORD));
#ifdef __BIGENDIAN__
    *lpData = MAKELONG(
        MAKEWORD(HIBYTE(HIWORD(*lpData)), LOBYTE(HIWORD(*lpData))),
	MAKEWORD(HIBYTE(LOWORD(*lpData)), LOBYTE(LOWORD(*lpData))));
#endif
    return AUDIO_ERROR_NONE;
}

/* big-endian input routines */
UINT AIAPI AIOReadCharM(LPBYTE lpData)
{
    if (fread(lpData, 1, sizeof(BYTE), lpStream) != sizeof(BYTE))
        memset(lpData, 0, sizeof(BYTE));
    return AUDIO_ERROR_NONE;
}

UINT AIAPI AIOReadShortM(LPWORD lpData)
{
    if (fread(lpData, 1, sizeof(WORD), lpStream) != sizeof(WORD))
        memset(lpData, 0, sizeof(WORD));
#ifndef __BIGENDIAN__
    *lpData = MAKEWORD(HIBYTE(*lpData), LOBYTE(*lpData));
#endif
    return AUDIO_ERROR_NONE;
}

UINT AIAPI AIOReadLongM(LPDWORD lpData)
{
    if (fread(lpData, 1, sizeof(DWORD), lpStream) != sizeof(DWORD))
        memset(lpData, 0, sizeof(DWORD));
#ifndef __BIGENDIAN__
    *lpData = MAKELONG(
        MAKEWORD(HIBYTE(HIWORD(*lpData)), LOBYTE(HIWORD(*lpData))),
	MAKEWORD(HIBYTE(LOWORD(*lpData)), LOBYTE(LOWORD(*lpData))));
#endif
    return AUDIO_ERROR_NONE;
}
