/*
 * $Id: iofile.h 1.4 1996/05/24 08:30:44 chasan released $
 *
 * Input/Output file stream routines.
 *
 * Copyright (C) 1995, 1996 Carlos Hasan. All Rights Reserved.
 *
 */

#ifndef __IOFILE_H
#define __IOFILE_H

#ifdef __GNUC__
#include <unistd.h>
#include <memory.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

UINT AIAPI AIOOpenFile(PSZ pszFileName);
UINT AIAPI AIOCloseFile(VOID);
UINT AIAPI AIOSeekFile(LONG dwOffset, UINT nWhere);
UINT AIAPI AIOReadFile(PVOID pData, UINT nSize);
UINT AIAPI AIOReadChar(PUCHAR pData);
UINT AIAPI AIOReadShort(PUSHORT pData);
UINT AIAPI AIOReadLong(PULONG pData);
UINT AIAPI AIOReadCharM(PUCHAR pData);
UINT AIAPI AIOReadShortM(PUSHORT pData);
UINT AIAPI AIOReadLongM(PULONG pData);

#ifdef __cplusplus
};
#endif

#endif
