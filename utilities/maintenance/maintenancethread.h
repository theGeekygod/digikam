/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2013-08-09
 * Description : Thread actions manager for maintenance tools.
 *
 * Copyright (C) 2013-2017 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#ifndef MAINTENANCE_THREAD_H
#define MAINTENANCE_THREAD_H

// Local includes

#include "actionthreadbase.h"
#include "metadatasynchronizer.h"
#include "imageinfo.h"
#include "identity.h"

class QImage;

namespace Digikam
{

class ImageQualitySettings;

class MaintenanceThread : public ActionThreadBase
{
    Q_OBJECT

public:

    explicit MaintenanceThread(QObject* const parent);
    ~MaintenanceThread();

    void setUseMultiCore(const bool b);

    void syncMetadata(const ImageInfoList& items, MetadataSynchronizer::SyncDirection dir, bool tagsOnly);
    void generateThumbs(const QStringList& paths);
    void generateFingerprints(const QStringList& paths, int chunkSize=0);
    void sortByImageQuality(const QStringList& paths, const ImageQualitySettings& quality);

    void computeDatabaseJunk(bool thumbsDb=false, bool facesDb=false);
    void cleanCoreDb(const QList<qlonglong>& imageIds, int chunkSize);
    void cleanThumbsDb(const QList<int>& thumbnailIds, int chunkSize);
    void cleanFacesDb(const QList<FacesEngine::Identity>& staleIdentities, int chunkSize);
    void shrinkDatabases();

    void cancel();

Q_SIGNALS:

    /** Emit when an item have been processed. QImage can be used to pass item thumbnail processed.
     */
    void signalAdvance(const QImage&);

    /** Emit when an itam was processed and on additional information is necessary.
     */
    void signalAdvance();

    /** Emit when a items list have been fully processed.
     */
    void signalCompleted();

    /** Signal to emit to sub-tasks to cancel processing.
     */
    void signalCanceled();

    /** Signal to emit junk data for db cleaner.
     */
    void signalData(const QList<qlonglong>& staleImageIds,
                    const QList<int>& staleThumbIds,
                    const QList<FacesEngine::Identity>& staleIdentities);

private Q_SLOTS:

    void slotThreadFinished();

};

}  // namespace Digikam

#endif /* MAINTENANCE_THREAD_H */
