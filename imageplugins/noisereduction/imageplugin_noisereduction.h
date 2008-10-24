/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2004-08-24
 * Description : a plugin to reduce CCD noise.
 *
 * Copyright (C) 2004-2008 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#ifndef IMAGEPLUGIN_NOISEREDUCTION_H
#define IMAGEPLUGIN_NOISEREDUCTION_H

// Qt includes.

#include <QVariant>

// Digikam includes.

#include "imageplugin.h"
#include "digikam_export.h"

class KAction;

class ImagePlugin_NoiseReduction : public Digikam::ImagePlugin
{
    Q_OBJECT

public:

    ImagePlugin_NoiseReduction(QObject *parent, const QVariantList &args);
    ~ImagePlugin_NoiseReduction();

    void setEnabledActions(bool enable);

private slots:

    void slotNoiseReduction();

private:

    KAction *m_noiseReductionAction;
};

#endif /* IMAGEPLUGIN_NOISEREDUCTION_H */
