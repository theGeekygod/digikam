/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2004-07-09
 * Description : a kio-slave to process tag query on
 *               digiKam albums.
 *
 * Copyright (C) 2004 by Renchi Raju <renchi@pooh.tam.uiuc.edu>
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

// Local includes.

#include "digikamtags.h"

// C++ includes.

#include <cstdlib>

// Qt includes.

#include <QCoreApplication>

// KDE includes.

#include <kcomponentdata.h>
#include <kdebug.h>
#include <kurl.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kio/global.h>

// Digikam includes.

#include "digikam_export.h"
#include "databaseaccess.h"
#include "databaseurl.h"
#include "albumdb.h"
#include "imagelister.h"
#include "imagelisterreceiver.h"

kio_digikamtagsProtocol::kio_digikamtagsProtocol(const QByteArray &pool_socket,
                                                 const QByteArray &app_socket)
                       : SlaveBase("kio_digikamtags", pool_socket, app_socket)
{
}

kio_digikamtagsProtocol::~kio_digikamtagsProtocol()
{
}

void kio_digikamtagsProtocol::special(const QByteArray& data)
{
    KUrl    kurl;
    QString filter;

    QDataStream ds(data);
    ds >> kurl;

    Digikam::DatabaseUrl dbUrl(kurl);
    Digikam::DatabaseAccess::setParameters(dbUrl);

    bool folders = (metaData("folders") == "true");

    if (folders)
    {
        QMap<int, int> tagNumberMap = Digikam::DatabaseAccess().db()->getNumberOfImagesInTags();

        QByteArray  ba;
        QDataStream os(&ba, QIODevice::WriteOnly);
        os << tagNumberMap;
        SlaveBase::data(ba);
    }
    else
    {
        bool recursive = (metaData("listTagsRecursively") == "true");

        Digikam::ImageLister lister;
        lister.setRecursive(recursive);
        // send data every 200 images to be more responsive
        Digikam::ImageListerSlaveBasePartsSendingReceiver receiver(this, 200);
        lister.list(&receiver, kurl);
        // send rest
        receiver.sendData();
    }

    finished();
}

/* KIO slave registration */

extern "C"
{
    KDE_EXPORT int kdemain(int argc, char **argv)
    {
        // Needed to load SQL driver plugins
        QCoreApplication app(argc, argv);

        KLocale::setMainCatalog("digikam");
        KComponentData componentData( "kio_digikamtags" );
        ( void ) KGlobal::locale();

        kDebug(50004) << "*** kio_digikamtag started ***";

        if (argc != 4) {
            kDebug(50004) << "Usage: kio_digikamtags  protocol domain-socket1 domain-socket2"
                      << endl;
            exit(-1);
        }

        kio_digikamtagsProtocol slave(argv[2], argv[3]);
        slave.dispatchLoop();

        kDebug(50004) << "*** kio_digikamtags finished ***";
        return 0;
    }
}

