/* ============================================================
 * Authors: Renchi Raju <renchi@pooh.tam.uiuc.edu>
 *          Caulier Gilles <caulier dot gilles at kdemail dot net>
 * Date  : 2004-09-18
 * Description : camera item info container
 * 
 * Copyright 2004-2005 by Renchi Raju
 * Copyright 2006 by Gilles Caulier
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * ============================================================ */

#ifndef GPITEMINFO_H
#define GPITEMINFO_H

// C++ includes.

#include <ctime>

// Qt includes.

#include <qvaluelist.h>
#include <qcstring.h>

class QDataStream;

namespace Digikam
{

class GPItemInfo
{

public:

    enum DownloadStatus
    {
        DownloadUnknow  = -1,
        DownloadedNo    = 0,
        DownloadedYes   = 1,
        DownloadFailed  = 2,
        DownloadStarted = 3,
    };

public:
    
    long    size;
    int     width;
    int     height;
    int     downloaded;           // See DownloadStatus enum.
    int     readPermissions;
    int     writePermissions;

    QString name;
    QString folder;
    QString mime;

    time_t  mtime;
};

QDataStream& operator<<( QDataStream &, const GPItemInfo & );
QDataStream& operator>>( QDataStream &, GPItemInfo & );

typedef QValueList<GPItemInfo> GPItemInfoList;

}  // namespace Digikam

#endif /* GPITEMINFO_H */
