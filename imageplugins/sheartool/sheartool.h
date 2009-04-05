/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2004-12-23
 * Description : a plugin to shear an image
 *
 * Copyright (C) 2004-2007 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#ifndef SHEARTOOL_H
#define SHEARTOOL_H

// Local includes

#include "editortool.h"

class QCheckBox;
class QLabel;

namespace Digikam
{
class ImageWidget;
class EditorToolSettings;
}

namespace KDcrawIface
{
class RIntNumInput;
class RDoubleNumInput;
}

namespace DigikamShearToolImagesPlugin
{

class ShearTool : public Digikam::EditorToolThreaded
{
    Q_OBJECT

public:

    ShearTool(QObject* parent);
    ~ShearTool();

private Q_SLOTS:

    void slotResetSettings();
    void slotColorGuideChanged();

private:

    void readSettings();
    void writeSettings();
    void prepareEffect(void);
    void prepareFinal(void);
    void putPreviewData(void);
    void putFinalData(void);
    void renderingFinished(void);

private:

    QLabel                       *m_newWidthLabel;
    QLabel                       *m_newHeightLabel;

    QCheckBox                    *m_antialiasInput;

    KDcrawIface::RIntNumInput    *m_mainHAngleInput;
    KDcrawIface::RIntNumInput    *m_mainVAngleInput;

    KDcrawIface::RDoubleNumInput *m_fineHAngleInput;
    KDcrawIface::RDoubleNumInput *m_fineVAngleInput;

    Digikam::ImageWidget         *m_previewWidget;

    Digikam::EditorToolSettings  *m_gboxSettings;
};

}  // namespace DigikamShearToolImagesPlugin

#endif /* SHEARTOOL_H */
