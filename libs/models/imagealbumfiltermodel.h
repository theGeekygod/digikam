/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-03-11
 * Description : Qt item model for database entries, using AlbumManager
 *
 * Copyright (C) 2009 by Marcel Wiesweg <marcel dot wiesweg at gmx dot de>
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

#ifndef IMAGEALBUMFILTERMODEL_H
#define IMAGEALBUMFILTERMODEL_H

// Qt includes

// Local includes

#include "imagefiltermodel.h"

namespace Digikam
{

class Album;
class ImageAlbumModel;
class ImageAlbumFilterModelPrivate;

class ImageAlbumFilterModel : public ImageFilterModel
{
    Q_OBJECT

public:

    ImageAlbumFilterModel(QObject *parent = 0);
    ~ImageAlbumFilterModel();

    virtual void setImageFilterSettings(const ImageFilterSettings &settings);

    void setSourceImageModel(ImageAlbumModel* model);

protected Q_SLOTS:

    //void slotListedAlbumChanged(Album *album);
    void slotAlbumRenamed(Album *album);
    void slotAlbumAdded(Album *album);
    void slotAlbumAboutToBeDeleted(Album *album);

private:

    void albumChange(Album *album);

    Q_DECLARE_PRIVATE(ImageAlbumFilterModel)
};

} // namespace Digikam

#endif // IMAGEALBUMFILTERMODEL_H
