/*
 * $Id: iofile.h 1.6 1996/09/13 16:30:26 chasan released $
 *
 * Input/Output file stream routines.
 *
 * Copyright (C) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __IOFILE_H
#define __IOFILE_H

#ifdef __GNUC__
#include <unistd.h>
#include <memory.h>
#else
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

    UINT AIAPI AIOOpenFile(LPSTR lpszFileName);
    UINT AIAPI AIOCloseFile(VOID);
    UINT AIAPI AIOSeekFile(LONG dwOffset, UINT nWhere);
    UINT AIAPI AIOReadFile(LPVOID lpData, UINT nSize);
    UINT AIAPI AIOReadChar(LPBYTE lpData);
    UINT AIAPI AIOReadShort(LPWORD lpData);
    UINT AIAPI AIOReadLong(LPDWORD lpData);
    UINT AIAPI AIOReadCharM(LPBYTE lpData);
    UINT AIAPI AIOReadShortM(LPWORD lpData);
    UINT AIAPI AIOReadLongM(LPDWORD lpData);

#ifdef __cplusplus
};
#endif

#endif
