/*
 * $Id: iofile.c 1.4 1996/05/24 08:30:44 chasan released $
 *
 * Input/Output file stream routines.
 *
 * Copyright (c) 1995, 1996 Carlos Hasan. All Rights Reserved.
 *
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
static FILE *fpIOFile;

UINT AIAPI AIOOpenFile(PSZ pszFileName)
{
    if ((fpIOFile = fopen(pszFileName, "rb")) != NULL)
        return AUDIO_ERROR_NONE;
    return AUDIO_ERROR_FILENOTFOUND;
}

UINT AIAPI AIOCloseFile(VOID)
{
    fclose(fpIOFile);
    return AUDIO_ERROR_NONE;
}

UINT AIAPI AIOSeekFile(LONG dwOffset, UINT nWhere)
{
    fseek(fpIOFile, dwOffset, nWhere);
    return AUDIO_ERROR_NONE;
}

UINT AIAPI AIOReadFile(PVOID pData, UINT nSize)
{
    if (fread(pData, 1, nSize, fpIOFile) != nSize)
        memset(pData, 0, nSize);
    return AUDIO_ERROR_NONE;
}

/* little-endian input routines */
UINT AIAPI AIOReadChar(PUCHAR pData)
{
    if (fread(pData, 1, sizeof(UCHAR), fpIOFile) != sizeof(UCHAR))
        memset(pData, 0, sizeof(UCHAR));
    return AUDIO_ERROR_NONE;
}

UINT AIAPI AIOReadShort(PUSHORT pData)
{
    if (fread(pData, 1, sizeof(USHORT), fpIOFile) != sizeof(USHORT))
        memset(pData, 0, sizeof(USHORT));
#ifdef __BIGENDIAN__
    *pData = MAKEWORD(HIBYTE(*pData), LOBYTE(*pData));
#endif
    return AUDIO_ERROR_NONE;
}

UINT AIAPI AIOReadLong(PULONG pData)
{
    if (fread(pData, 1, sizeof(ULONG), fpIOFile) != sizeof(ULONG))
        memset(pData, 0, sizeof(ULONG));
#ifdef __BIGENDIAN__
    *pData = MAKELONG(
        MAKEWORD(HIBYTE(HIWORD(*pData)), LOBYTE(HIWORD(*pData))),
            MAKEWORD(HIBYTE(LOWORD(*pData)), LOBYTE(LOWORD(*pData))));
#endif
    return AUDIO_ERROR_NONE;
}

/* big-endian input routines */
UINT AIAPI AIOReadCharM(PUCHAR pData)
{
    if (fread(pData, 1, sizeof(UCHAR), fpIOFile) != sizeof(UCHAR))
        memset(pData, 0, sizeof(UCHAR));
    return AUDIO_ERROR_NONE;
}

UINT AIAPI AIOReadShortM(PUSHORT pData)
{
    if (fread(pData, 1, sizeof(USHORT), fpIOFile) != sizeof(USHORT))
        memset(pData, 0, sizeof(USHORT));
#ifndef __BIGENDIAN__
    *pData = MAKEWORD(HIBYTE(*pData), LOBYTE(*pData));
#endif
    return AUDIO_ERROR_NONE;
}

UINT AIAPI AIOReadLongM(PULONG pData)
{
    if (fread(pData, 1, sizeof(ULONG), fpIOFile) != sizeof(ULONG))
        memset(pData, 0, sizeof(ULONG));
#ifndef __BIGENDIAN__
    *pData = MAKELONG(
        MAKEWORD(HIBYTE(HIWORD(*pData)), LOBYTE(HIWORD(*pData))),
            MAKEWORD(HIBYTE(LOWORD(*pData)), LOBYTE(LOWORD(*pData))));
#endif
    return AUDIO_ERROR_NONE;
}
