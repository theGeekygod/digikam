/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2012-12-05
 * Description : a plugin to create panorama by fusion of several images.
 *
 * Copyright (C) 2012-2015 by Benjamin Girault <benjamin dot girault at gmail dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#ifndef CREATEPFINALPTOTASK_H
#define CREATEPFINALPTOTASK_H

// Local includes

#include "panotask.h"
#include "ptotype.h"

namespace Digikam
{

class CreateFinalPtoTask : public PanoTask
{
private:

    PTOType                 ptoData;
    QUrl&                   finalPtoUrl;
    const QRect             crop;

public:

    CreateFinalPtoTask(const QString& workDirPath, QSharedPointer<const PTOType> ptoData, QUrl& finalPtoUrl, const QRect& crop);
    ~CreateFinalPtoTask();

protected:

    void run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread) override;
};

}  // namespace Digikam

#endif /* CREATEPFINALPTOTASK_H */
