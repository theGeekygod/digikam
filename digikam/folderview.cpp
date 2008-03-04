/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2005-05-06
 * Description : implementation of folder view.
 *
 * Copyright (C) 2005-2006 by Joern Ahrens <joern.ahrens@kdemail.net>
 * Copyright (C) 2006-2008 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

// Qt includes.

#include <qpixmap.h>
#include <qvaluelist.h>

// KDE includes.

#include <kglobalsettings.h>
#include <kcursor.h>
#include <kapplication.h>
#include <kconfig.h>

// Local includes.

#include "ddebug.h"
#include "albummanager.h"
#include "albumsettings.h"
#include "albumthumbnailloader.h"
#include "themeengine.h"
#include "dragobjects.h"
#include "folderitem.h"
#include "folderview.h"
#include "folderview.moc"

namespace Digikam
{

class FolderViewPriv
{
public:

    FolderViewPriv()
    {
        active           = false;
        dragItem         = 0;
        oldHighlightItem = 0;
    }

    bool           active;

    int            itemHeight;

    QPixmap        itemRegPix;
    QPixmap        itemSelPix;

    QPoint         dragStartPos;

    QListViewItem *dragItem;
    QListViewItem *oldHighlightItem;
};

//-----------------------------------------------------------------------------

FolderView::FolderView(QWidget *parent, const char *name)
          : QListView(parent, name)
{

    d = new FolderViewPriv;

    connect(ThemeEngine::instance(), SIGNAL(signalThemeChanged()),
            this, SLOT(slotThemeChanged()));

    connect(AlbumManager::instance(), SIGNAL(signalAllAlbumsLoaded()),
            this, SLOT(slotAllAlbumsLoaded()));

    connect(AlbumThumbnailLoader::instance(), SIGNAL(signalReloadThumbnails()),
            this, SLOT(slotIconSizeChanged()));

    setColumnAlignment(0, Qt::AlignLeft|Qt::AlignVCenter);
    fontChange(font());
}

FolderView::~FolderView()
{
    saveViewState();
    delete d;
}

void FolderView::setActive(bool val)
{
    d->active = val;

    if (d->active)
        slotSelectionChanged();
}

bool FolderView::active() const
{
    return d->active;
}

int FolderView::itemHeight() const
{
    return d->itemHeight;
}

QRect FolderView::itemRect(QListViewItem *item) const
{
    if(!item)
        return QRect();

    QRect r = QListView::itemRect(item);
    r.setLeft(r.left()+(item->depth()+(rootIsDecorated() ? 1 : 0))*treeStepSize());
    return r;
}

QPixmap FolderView::itemBasePixmapRegular() const
{
    return d->itemRegPix;
}

QPixmap FolderView::itemBasePixmapSelected() const
{
    return d->itemSelPix;
}

void FolderView::resizeEvent(QResizeEvent* e)
{
    QListView::resizeEvent(e);

    int w = frameRect().width();
    int h = itemHeight();
    if (d->itemRegPix.width() != w ||
        d->itemRegPix.height() != h)
    {
        slotThemeChanged();
    }
}

void FolderView::fontChange(const QFont& oldFont)
{
    // this is bad, since the settings value might not always be the _real_ height of the thumbnail.
    // (e.g. when it is blended, as for the tags)
    d->itemHeight = QMAX(AlbumThumbnailLoader::instance()->thumbnailSize() + 2*itemMargin(), fontMetrics().height());
    QListView::fontChange(oldFont);
    slotThemeChanged();
}

void FolderView::slotIconSizeChanged()
{
    d->itemHeight = QMAX(AlbumThumbnailLoader::instance()->thumbnailSize() + 2*itemMargin(), fontMetrics().height());
    slotThemeChanged();
}

void FolderView::contentsMouseMoveEvent(QMouseEvent *e)
{
    QListView::contentsMouseMoveEvent(e);

    if(e->state() == NoButton)
    {
        if(KGlobalSettings::changeCursorOverIcon())
        {
            QPoint vp = contentsToViewport(e->pos());
            QListViewItem *item = itemAt(vp);
            if (mouseInItemRect(item, vp.x()))
                setCursor(KCursor::handCursor());
            else
                unsetCursor();
        }
        return;
    }

    if(d->dragItem && 
       (d->dragStartPos - e->pos()).manhattanLength() > QApplication::startDragDistance())
    {
        QPoint vp = contentsToViewport(e->pos());
        FolderItem *item = dynamic_cast<FolderItem*>(itemAt(vp));
        if(!item)
        {
            d->dragItem = 0;
            return;
        }
    }
}

void FolderView::contentsMousePressEvent(QMouseEvent *e)
{
    QPoint vp           = contentsToViewport(e->pos());
    QListViewItem *item = itemAt(vp);

    FolderCheckListItem *citem = dynamic_cast<FolderCheckListItem*>(item);
    if(citem && e->button() == MidButton) 
    {
        bool isOn = citem->isOn();
        QListView::contentsMousePressEvent(e);
        // Restore the status of checkbox. 
        citem->setOn(isOn);
        return;
    }

    QListView::contentsMousePressEvent(e);

    if(item && e->button() == LeftButton) 
    {
        d->dragStartPos = e->pos();
        d->dragItem     = item;
        return;
    }
}

void FolderView::contentsMouseReleaseEvent(QMouseEvent *e)
{
    QListView::contentsMouseReleaseEvent(e);

    d->dragItem = 0;
}

void FolderView::startDrag()
{
    QDragObject *o = dragObject();
    if(o)
        o->drag();
}

QListViewItem* FolderView::dragItem() const
{
    return d->dragItem;
}

void FolderView::contentsDragEnterEvent(QDragEnterEvent *e)
{
    QListView::contentsDragEnterEvent(e);

    e->accept(acceptDrop(e));
}

void FolderView::contentsDragLeaveEvent(QDragLeaveEvent * e)
{
    QListView::contentsDragLeaveEvent(e);

    if(d->oldHighlightItem)
    {
        FolderItem *fitem = dynamic_cast<FolderItem*>(d->oldHighlightItem);
        if (fitem)
            fitem->setFocus(false);
        else
        {
            FolderCheckListItem *citem = dynamic_cast<FolderCheckListItem*>(d->oldHighlightItem);
            if (citem)
                citem->setFocus(false);
        }
        d->oldHighlightItem->repaint();
        d->oldHighlightItem = 0;
    }
}

void FolderView::contentsDragMoveEvent(QDragMoveEvent *e)
{
    QListView::contentsDragMoveEvent(e);

    QPoint vp           = contentsToViewport(e->pos());
    QListViewItem *item = itemAt(vp);
    if(item)
    {
        if(d->oldHighlightItem)
        {
            FolderItem *fitem = dynamic_cast<FolderItem*>(d->oldHighlightItem);
            if (fitem)
                fitem->setFocus(false);
            else
            {
                FolderCheckListItem *citem = dynamic_cast<FolderCheckListItem*>(d->oldHighlightItem);
                if (citem)
                    citem->setFocus(false);
            }
            d->oldHighlightItem->repaint();
        }

        FolderItem *fitem = dynamic_cast<FolderItem*>(item);
        if (fitem)
            fitem->setFocus(true);
        else
        {
            FolderCheckListItem *citem = dynamic_cast<FolderCheckListItem*>(item);
            if (citem)
                citem->setFocus(true);
        }
        d->oldHighlightItem = item;
        item->repaint();
    }
    e->accept(acceptDrop(e));
}

void FolderView::contentsDropEvent(QDropEvent *e)
{
    QListView::contentsDropEvent(e);

    if(d->oldHighlightItem)
    {
        FolderItem *fitem = dynamic_cast<FolderItem*>(d->oldHighlightItem);
        if (fitem)
            fitem->setFocus(false);
        else
        {
            FolderCheckListItem *citem = dynamic_cast<FolderCheckListItem*>(d->oldHighlightItem);
            if (citem)
                citem->setFocus(false);
        }
        d->oldHighlightItem->repaint();
        d->oldHighlightItem = 0;
    }
}

bool FolderView::acceptDrop(const QDropEvent *) const
{
    return false;
}

bool FolderView::mouseInItemRect(QListViewItem* item, int x) const
{
    if (!item)
        return false;

    x += contentsX();

    int offset = treeStepSize()*(item->depth() + (rootIsDecorated() ? 1 : 0));
    offset    += itemMargin();
    int width  = item->width(fontMetrics(), this, 0);

    return (x > offset && x < (offset + width));
}

void FolderView::slotThemeChanged()
{
    int w = frameRect().width();
    int h = itemHeight();

    d->itemRegPix = ThemeEngine::instance()->listRegPixmap(w, h);
    d->itemSelPix = ThemeEngine::instance()->listSelPixmap(w, h);

    QPalette plt(palette());
    QColorGroup cg(plt.active());
    cg.setColor(QColorGroup::Base, ThemeEngine::instance()->baseColor());
    cg.setColor(QColorGroup::Text, ThemeEngine::instance()->textRegColor());
    cg.setColor(QColorGroup::HighlightedText, ThemeEngine::instance()->textSelColor());
    cg.setColor(QColorGroup::Link, ThemeEngine::instance()->textSpecialRegColor());
    cg.setColor(QColorGroup::LinkVisited, ThemeEngine::instance()->textSpecialSelColor());
    plt.setActive(cg);
    plt.setInactive(cg);
    setPalette(plt);

    viewport()->update();
}

void FolderView::slotAllAlbumsLoaded()
{
    disconnect(AlbumManager::instance(), SIGNAL(signalAllAlbumsLoaded()),
               this, SLOT(slotAllAlbumsLoaded()));
    loadViewState();
}

void FolderView::loadViewState()
{
    KConfig *config = kapp->config();
    config->setGroup(name());

    int selectedItem = config->readNumEntry("LastSelectedItem", 0);

    QValueList<int> openFolders;
    if(config->hasKey("OpenFolders"))
    {
        openFolders = config->readIntListEntry("OpenFolders");
    }

    FolderItem *item      = 0;
    FolderItem *foundItem = 0;
    QListViewItemIterator it(this->lastItem());

    for( ; it.current(); --it)
    {
        item = dynamic_cast<FolderItem*>(it.current());
        if(!item)
            continue;

        // Start the album root always open
        if(openFolders.contains(item->id()) || item->id() == 0)
            setOpen(item, true);
        else
            setOpen(item, false);

        if(item->id() == selectedItem)
        {
            // Save the found selected item so that it can be made visible.
            foundItem = item;
        }
    }

    // Important note: this cannot be done inside the previous loop
    // because opening folders prevents the visibility.
    // Fixes bug #144815.
    // (Looks a bit like a bug in Qt to me ...)
    if (foundItem)
    {
        setSelected(foundItem, true);
        ensureItemVisible(foundItem);
    }
}

void FolderView::saveViewState()
{
    KConfig *config = kapp->config();
    config->setGroup(name());

    FolderItem *item = dynamic_cast<FolderItem*>(selectedItem());
    if(item)
        config->writeEntry("LastSelectedItem", item->id());
    else
        config->writeEntry("LastSelectedItem", 0);

    QValueList<int> openFolders;
    QListViewItemIterator it(this);
    for( ; it.current(); ++it)
    {
        item = dynamic_cast<FolderItem*>(it.current());
        if(item && isOpen(item))
            openFolders.push_back(item->id());
    }
    config->writeEntry("OpenFolders", openFolders);
}

void FolderView::slotSelectionChanged()
{
    QListView::selectionChanged();
}

void FolderView::selectItem(int)
{
}

}  // namespace Digikam
