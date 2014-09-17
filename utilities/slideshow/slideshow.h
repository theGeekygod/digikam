/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2005-04-21
 * Description : slide show tool using preview of pictures.
 *
 * Copyright (C) 2005-2014 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#ifndef SLIDE_SHOW_H
#define SLIDE_SHOW_H

// Qt includes

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWheelEvent>
#include <QWidget>

// Local includes

#include "digikam_export.h"
#include "loadingdescription.h"
#include "slideshowsettings.h"

namespace Digikam
{

class DImg;

class DIGIKAM_EXPORT SlideShow : public QWidget
{
    Q_OBJECT

public:

    explicit SlideShow(const SlideShowSettings& settings);
    ~SlideShow();

    void setCurrentUrl(const KUrl& url);
    KUrl currentUrl() const;

    void toggleTag(int tag);
    void updateTags(const KUrl& url, const QStringList& tags);

Q_SIGNALS:

    void signalRatingChanged(const KUrl&, int);
    void signalColorLabelChanged(const KUrl&, int);
    void signalPickLabelChanged(const KUrl&, int);
    void signalToggleTag(const KUrl&, int);

public Q_SLOTS:

    void slotAssignRating(int);
    void slotAssignColorLabel(int);
    void slotAssignPickLabel(int);

protected:

    void paintEvent(QPaintEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void keyPressEvent(QKeyEvent*);
    void wheelEvent(QWheelEvent*);
    bool eventFilter(QObject* obj, QEvent* ev);

private Q_SLOTS:

    void slotTimeOut();
    void slotMouseMoveTimeOut();
    void slotGotImagePreview(const LoadingDescription&, const DImg&);

    void slotPause();
    void slotPlay();
    void slotPrev();
    void slotNext();
    void slotClose();

private:

    void loadNextImage();
    void loadPrevImage();
    void preloadNextImage();
    void updatePixmap();
    void printInfoText(QPainter& p, int& offset, const QString& str);
    void printComments(QPainter& p, int& offset, const QString& comments);
    void printTags(QStringList& tags);
    void inhibitScreenSaver();
    void allowScreenSaver();
    void makeCornerRectangles(const QRect& desktopRect, const QSize& size,
                              QRect* const topLeft, QRect* const topRight,
                              QRect* const topLeftLarger, QRect* const topRightLarger);

private:

    class Private;
    Private* const d;
};

}  // namespace Digikam

#endif /* SLIDE_SHOW_H */
