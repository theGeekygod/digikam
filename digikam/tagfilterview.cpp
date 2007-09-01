/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2005-05-05
 * Description : tags filter view
 *
 * Copyright (C) 2005 by Renchi Raju <renchi@pooh.tam.uiuc.edu>
 * Copyright (C) 2006-2007 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#include <Q3ValueList>
#include <Q3Header>
#include <QPixmap>
#include <QPainter>
#include <QTimer>
#include <QCursor>
#include <QDropEvent>
#include <QMouseEvent>

// KDE includes.

#include <kmenu.h>
#include <klocale.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <kglobalsettings.h>
#include <kcursor.h>
#include <kmessagebox.h>
#include <kglobal.h>
#include <kselectaction.h>

// Local includes.

#include "ddebug.h"
#include "albummanager.h"
#include "albumdb.h"
#include "album.h"
#include "albumlister.h"
#include "albumthumbnailloader.h"
#include "databasetransaction.h"
#include "syncjob.h"
#include "dragobjects.h"
#include "folderitem.h"
#include "imageattributeswatch.h"
#include "imageinfo.h"
#include "metadatahub.h"
#include "tagcreatedlg.h"
#include "statusprogressbar.h"
#include "tagfilterview.h"
#include "tagfilterview.moc"

#include "config.h"
#ifdef KDEPIMLIBS_FOUND
#include <kabc/stdaddressbook.h>
#endif

namespace Digikam
{

class TagFilterViewItem : public FolderCheckListItem
{

public:

    TagFilterViewItem(Q3ListView* parent, TAlbum* tag, bool untagged=false)
        : FolderCheckListItem(parent, tag ? tag->title() : i18n("Not Tagged"),
                              Q3CheckListItem::CheckBox/*Controller*/)
    {
        m_tag      = tag;
        m_untagged = untagged;
        setDragEnabled(!untagged);

        if (tag)
            tag->setExtraData(listView(), this);
    }

    TagFilterViewItem(Q3ListViewItem* parent, TAlbum* tag)
        : FolderCheckListItem(parent, tag->title(), 
                              Q3CheckListItem::CheckBox/*Controller*/)
    {
        m_tag      = tag;
        m_untagged = false;
        setDragEnabled(true);

        if (tag)
            tag->setExtraData(listView(), this);
    }

    virtual void stateChange(bool val)
    {
        Q3CheckListItem::stateChange(val);

/* NOTE G.Caulier 2007/01/08: this code is now disable because TagFilterViewItem 
                              have been changed from QCheckListItem::CheckBoxController 
                              to QCheckListItem::CheckBox.
 
        // All TagFilterViewItems are CheckBoxControllers. If they have no children,
        // they should be of type CheckBox, but that is not possible with our way of adding items.
        // When clicked, children-less items first change to the NoChange state, and a second
        // click is necessary to set them to On and make the filter take effect.
        // So set them to On if the condition is met.
        if (!firstChild() && state() == NoChange)
        {
            setState(On);
        }
*/

        ((TagFilterView*)listView())->stateChanged(this);
    }

    int compare(Q3ListViewItem* i, int column, bool ascending) const
    {
        if (m_untagged)
            return 1;

        TagFilterViewItem* dItem = dynamic_cast<TagFilterViewItem*>(i);
        if (!dItem)
            return 0;

        if (dItem && dItem->m_untagged)
            return -1;

        return Q3ListViewItem::compare(i, column, ascending);
    }

    void paintCell(QPainter* p, const QColorGroup & cg, int column, int width, int align)
    {
        if (!m_untagged)
        {
            FolderCheckListItem::paintCell(p, cg, column, width, align);
            return;
        }

        QFont f(listView()->font());
        f.setBold(true);
        f.setItalic(true);
        p->setFont(f);

        QColorGroup mcg(cg);
        mcg.setColor(QColorGroup::Text, Qt::darkRed);

        FolderCheckListItem::paintCell(p, mcg, column, width, align);
    }

    TAlbum *m_tag;
    bool    m_untagged;
};

// ---------------------------------------------------------------------

class TagFilterViewPrivate
{

public:

    TagFilterViewPrivate()
    {
        dragItem       = 0;
        ABCMenu        = 0;
        timer          = 0;
        toggleAutoTags = TagFilterView::NoToggleAuto;
        matchingCond   = AlbumLister::OrCondition;
    }

    QTimer                         *timer;

    QPoint                          dragStartPos;    

    QMenu                          *ABCMenu;

    TagFilterView::ToggleAutoTags   toggleAutoTags;

    AlbumLister::MatchingCondition  matchingCond;
    
    TagFilterViewItem              *dragItem;
};


TagFilterView::TagFilterView(QWidget* parent)
             : FolderView(parent)
{
    d = new TagFilterViewPrivate;
    d->timer = new QTimer(this);

    addColumn(i18n("Tag Filters"));
    setResizeMode(Q3ListView::LastColumn);
    setRootIsDecorated(true);

    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);

    TagFilterViewItem* notTaggedItem = new TagFilterViewItem(this, 0, true);
    notTaggedItem->setPixmap(0, AlbumThumbnailLoader::componentData()->getStandardTagIcon());

    // -- setup slots ---------------------------------------------------------

    connect(AlbumManager::instance(), SIGNAL(signalAlbumAdded(Album*)),
            this, SLOT(slotTagAdded(Album*)));

    connect(AlbumManager::instance(), SIGNAL(signalAlbumDeleted(Album*)),
            this, SLOT(slotTagDeleted(Album*)));

    connect(AlbumManager::instance(), SIGNAL(signalAlbumRenamed(Album*)),
            this, SLOT(slotTagRenamed(Album*)));

    connect(AlbumManager::instance(), SIGNAL(signalAlbumsCleared()),
            this, SLOT(slotClear()));

    connect(AlbumManager::instance(), SIGNAL(signalAlbumIconChanged(Album*)),
            this, SLOT(slotAlbumIconChanged(Album*)));

    connect(AlbumManager::instance(), SIGNAL(signalTAlbumMoved(TAlbum*, TAlbum*)),
            this, SLOT(slotTagMoved(TAlbum*, TAlbum*)));

    AlbumThumbnailLoader *loader = AlbumThumbnailLoader::componentData();

    connect(loader, SIGNAL(signalThumbnail(Album *, const QPixmap&)),
            this, SLOT(slotGotThumbnailFromIcon(Album *, const QPixmap&)));

    connect(loader, SIGNAL(signalFailed(Album *)),
            this, SLOT(slotThumbnailLost(Album *)));

    connect(loader, SIGNAL(signalReloadThumbnails()),
            this, SLOT(slotReloadThumbnails()));

    connect(this, SIGNAL(contextMenuRequested(Q3ListViewItem*, const QPoint&, int)),
            this, SLOT(slotContextMenu(Q3ListViewItem*, const QPoint&, int)));

    connect(d->timer, SIGNAL(timeout()),
            this, SLOT(slotTimeOut()));

    // -- read config ---------------------------------------------------------

    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup group = config->group("Tag Filters View");
    d->matchingCond = (AlbumLister::MatchingCondition)(group.readEntry("Matching Condition", 
                                                       (int)AlbumLister::OrCondition));

    d->toggleAutoTags = (ToggleAutoTags)(group.readEntry("Toggle Auto Tags", (int)NoToggleAuto));
}

TagFilterView::~TagFilterView()
{
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup group = config->group("Tag Filters View");
    group.writeEntry("Matching Condition", (int)(d->matchingCond));
    group.writeEntry("Toggle Auto Tags", (int)(d->toggleAutoTags));
    config->sync();

    delete d->timer;
    delete d;
}

void TagFilterView::stateChanged(TagFilterViewItem* item)
{
    ToggleAutoTags oldAutoTags = d->toggleAutoTags;            

    switch(d->toggleAutoTags)
    {
        case Children:
            d->toggleAutoTags = TagFilterView::NoToggleAuto;
            toggleChildTags(item, item->isOn());
            d->toggleAutoTags = oldAutoTags;
            break;
        case Parents:
            d->toggleAutoTags = TagFilterView::NoToggleAuto;
            toggleParentTags(item, item->isOn());
            d->toggleAutoTags = oldAutoTags;
            break;
        case ChildrenAndParents:
            d->toggleAutoTags = TagFilterView::NoToggleAuto;
            toggleChildTags(item, item->isOn());
            toggleParentTags(item, item->isOn());
            d->toggleAutoTags = oldAutoTags;
            break;
        default:
            break;
    }

    triggerChange();
}

void TagFilterView::triggerChange()
{
    d->timer->setSingleShot(true);
    d->timer->start(50);
}

void TagFilterView::contentsMouseMoveEvent(QMouseEvent *e)
{
    Q3ListView::contentsMouseMoveEvent(e);

    if(e->buttons() == Qt::NoButton)
    {
        if(KGlobalSettings::changeCursorOverIcon())
        {
            QPoint vp = contentsToViewport(e->pos());
            Q3ListViewItem *item = itemAt(vp);
            if (mouseInItemRect(item, vp.x()))
                setCursor(Qt::PointingHandCursor);
            else
                unsetCursor();
        }
        return;
    }
    
    if(d->dragItem && 
       (d->dragStartPos - e->pos()).manhattanLength() > QApplication::startDragDistance())
    {
        QPoint vp = contentsToViewport(e->pos());
        TagFilterViewItem *item = dynamic_cast<TagFilterViewItem*>(itemAt(vp));
        if(!item)
        {
            d->dragItem = 0;
            return;
        }
    }    
}

void TagFilterView::contentsMousePressEvent(QMouseEvent *e)
{
    QPoint vp = contentsToViewport(e->pos());
    TagFilterViewItem *item = dynamic_cast<TagFilterViewItem*>(itemAt(vp));

    if(item && e->button() == Qt::RightButton) 
    {
        bool isOn = item->isOn();
        Q3ListView::contentsMousePressEvent(e);
        // Restore the status of checkbox. 
        item->setOn(isOn);
        return;
    }

    Q3ListView::contentsMousePressEvent(e);

    if(item && e->button() == Qt::LeftButton) 
    {
        d->dragStartPos = e->pos();
        d->dragItem     = item;
    }
}

void TagFilterView::contentsMouseReleaseEvent(QMouseEvent *e)
{
    Q3ListView::contentsMouseReleaseEvent(e);

    d->dragItem = 0;
}

Q3DragObject* TagFilterView::dragObject()
{
    TagFilterViewItem *item = dynamic_cast<TagFilterViewItem*>(dragItem());
    if(!item)
        return 0;

    TagDrag *t = new TagDrag(item->m_tag->id(), this);
    t->setPixmap(*item->pixmap(0));

    return t;
}

TagFilterViewItem* TagFilterView::dragItem() const
{
    return d->dragItem;
}

bool TagFilterView::acceptDrop(const QDropEvent *e) const
{
    QPoint vp = contentsToViewport(e->pos());
    TagFilterViewItem *itemDrop = dynamic_cast<TagFilterViewItem*>(itemAt(vp));
    TagFilterViewItem *itemDrag = dynamic_cast<TagFilterViewItem*>(dragItem());

    if(TagDrag::canDecode(e) || TagListDrag::canDecode(e))
    {
        // Allow dragging at the root, to move the tag to the root
        if(!itemDrop)
            return true;

        // Do not allow dragging at the "Not Tagged" item.
        if (itemDrop->m_untagged)
            return false;

        // Dragging an item on itself makes no sense
        if(itemDrag == itemDrop)
            return false;

        // Dragging a parent on its child makes no sense
        if(itemDrag && itemDrag->m_tag->isAncestorOf(itemDrop->m_tag))
            return false;

        return true;
    }

    if (ItemDrag::canDecode(e) && itemDrop && !itemDrop->m_untagged)
    {
        TAlbum *tag = itemDrop->m_tag;
        
        if (tag)
        {
            if (tag->parent())
            {         
                // Only other possibility is image items being dropped
                // And allow this only if there is a Tag to be dropped
                // on and also the Tag is not root or "Not Tagged" item.
                return true;
            }
        }
    }

    return false;
}

void TagFilterView::contentsDropEvent(QDropEvent *e)
{
    FolderView::contentsDropEvent(e);

    if (!acceptDrop(e))
        return;

    QPoint vp = contentsToViewport(e->pos());
    TagFilterViewItem *itemDrop = dynamic_cast<TagFilterViewItem*>(itemAt(vp));

    if (!itemDrop || itemDrop->m_untagged)
        return;

    if(TagDrag::canDecode(e))
    {
        QByteArray ba = e->encodedData("digikam/tag-id");
        QDataStream ds(&ba, QIODevice::ReadOnly);
        int tagID;
        ds >> tagID;

        AlbumManager* man = AlbumManager::instance();
        TAlbum* talbum    = man->findTAlbum(tagID);

        if(!talbum)
            return;

        if (talbum == itemDrop->m_tag)
            return;

        KMenu popMenu(this);
        popMenu.addTitle(SmallIcon("digikam"), i18n("Tag Filters"));
        QAction *gotoAction = popMenu.addAction(SmallIcon("goto"), i18n("&Move Here"));
        popMenu.addSeparator();
        popMenu.addAction(SmallIcon("cancel"), i18n("C&ancel"));
        popMenu.setMouseTracking(true);
        QAction *choice = popMenu.exec(QCursor::pos());

        if(choice == gotoAction)
        {
            TAlbum *newParentTag = 0;

            if (!itemDrop)
            {
                // move dragItem to the root
                newParentTag = AlbumManager::instance()->findTAlbum(0);
            }
            else
            {
                // move dragItem as child of dropItem
                newParentTag = itemDrop->m_tag;
            }

            QString errMsg;
            if (!AlbumManager::instance()->moveTAlbum(talbum, newParentTag, errMsg))
            {
                KMessageBox::error(this, errMsg);
            }

            if(itemDrop && !itemDrop->isOpen())
                itemDrop->setOpen(true);
        }

        return;
    }

    if (ItemDrag::canDecode(e))
    {
        TAlbum *destAlbum = itemDrop->m_tag;

        KUrl::List      urls;
        KUrl::List      kioURLs;
        Q3ValueList<int> albumIDs;
        Q3ValueList<int> imageIDs;

        if (!ItemDrag::decode(e, urls, kioURLs, albumIDs, imageIDs))
            return;

        if (urls.isEmpty() || kioURLs.isEmpty() || albumIDs.isEmpty() || imageIDs.isEmpty())
            return;


        // If a ctrl key is pressed while dropping the drag object,
        // the tag is assigned to the images without showing a
        // popup menu.
        bool assignTag = false, setThumbnail = false;
        if (e->keyboardModifiers() == Qt::ControlModifier)
        {
            assignTag = true;
        }
        else
        {
            KMenu popMenu(this);
            popMenu.addTitle(SmallIcon("digikam"), i18n("Tag Filters"));
            QAction *assignAction = popMenu.addAction(SmallIcon("tag"), i18n("Assign Tag '%1' to Items", destAlbum->prettyUrl()));
            QAction *thumbnailAction = popMenu.addAction(i18n("Set as Tag Thumbnail"));
            popMenu.addSeparator();
            popMenu.addAction(SmallIcon("cancel"), i18n("C&ancel"));

            popMenu.setMouseTracking(true);
            QAction *choice = popMenu.exec(QCursor::pos());
            assignTag = choice == assignAction;
            setThumbnail = choice == thumbnailAction;
        }

        if (assignTag)
        {
            emit signalProgressBarMode(StatusProgressBar::ProgressBarMode, 
                                       i18n("Assigning image tags. Please wait..."));

            {
                DatabaseTransaction transaction;
                int i=0;
                for (Q3ValueList<int>::const_iterator it = imageIDs.begin();
                    it != imageIDs.end(); ++it)
                {
                    // create temporary ImageInfo object
                    ImageInfo info(*it);

                    MetadataHub hub;
                    hub.load(info);
                    hub.setTag(destAlbum, true);
                    hub.write(info, MetadataHub::PartialWrite);
                    hub.write(info.filePath(), MetadataHub::FullWriteIfChanged);

                    emit signalProgressValue((int)((i++/(float)imageIDs.count())*100.0));
                    kapp->processEvents();
                }
            }

            emit signalProgressBarMode(StatusProgressBar::TextMode, QString());
        }
        else if(setThumbnail)
        {
            QString errMsg;
            AlbumManager::instance()->updateTAlbumIcon(destAlbum, QString(),
                                                       imageIDs.first(), errMsg);
        }
    }
}

void TagFilterView::slotTagAdded(Album* album)
{
    if (!album || album->isRoot())
        return;

    TAlbum* tag = dynamic_cast<TAlbum*>(album);
    if (!tag)
        return;

    if (tag->parent()->isRoot())
    {
        new TagFilterViewItem(this, tag);
    }
    else
    {
        TagFilterViewItem* parent = (TagFilterViewItem*)(tag->parent()->extraData(this));
        if (!parent)
        {
            DWarning() << " Failed to find parent for Tag "
                       << tag->tagPath() << endl;
            return;
        }

        new TagFilterViewItem(parent, tag);
    }

    setTagThumbnail(tag);
}

void TagFilterView::slotTagRenamed(Album* album)
{
    if (!album)
        return;

    TAlbum* tag = dynamic_cast<TAlbum*>(album);
    if (!tag)
        return;

    TagFilterViewItem* item = (TagFilterViewItem*)(tag->extraData(this));
    if (item)
    {
        item->setText(0, tag->title());
    }
}

void TagFilterView::slotTagMoved(TAlbum* tag, TAlbum* newParent)
{
    if (!tag || !newParent)
        return;

    TagFilterViewItem* item = (TagFilterViewItem*)(tag->extraData(this));
    if (!item)
        return;

    if (item->parent())
    {
        Q3ListViewItem* oldPItem = item->parent();
        oldPItem->takeItem(item);

        TagFilterViewItem* newPItem = (TagFilterViewItem*)(newParent->extraData(this));
        if (newPItem)
            newPItem->insertItem(item);
        else
            insertItem(item);
    }
    else
    {
        takeItem(item);

        TagFilterViewItem* newPItem = (TagFilterViewItem*)(newParent->extraData(this));

        if (newPItem)
            newPItem->insertItem(item);
        else
            insertItem(item);
    }
}

void TagFilterView::slotTagDeleted(Album* album)
{
    if (!album || album->isRoot())
        return;

    TAlbum* tag = dynamic_cast<TAlbum*>(album);
    if (!tag)
        return;

    TagFilterViewItem* item = (TagFilterViewItem*)(album->extraData(this));
    if (!item)
        return;

    album->removeExtraData(this);
    delete item;
}

void TagFilterView::setTagThumbnail(TAlbum *album)
{
    if(!album)
        return;

    TagFilterViewItem* item = (TagFilterViewItem*) album->extraData(this);

    if(!item)
        return;

    AlbumThumbnailLoader *loader = AlbumThumbnailLoader::componentData();
    QPixmap icon;
    if (!loader->getTagThumbnail(album, icon))
    {
        if (icon.isNull())
        {
            item->setPixmap(0, loader->getStandardTagIcon(album));
        }
        else
        {
            QPixmap blendedIcon = loader->blendIcons(loader->getStandardTagIcon(), icon);
            item->setPixmap(0, blendedIcon);
        }
    }
    else
    {
        // for the time being, set standard icon
        item->setPixmap(0, loader->getStandardTagIcon(album));
    }
}

void TagFilterView::slotGotThumbnailFromIcon(Album *album, const QPixmap& thumbnail)
{
    if(!album || album->type() != Album::TAG)
        return;

    TagFilterViewItem* item = (TagFilterViewItem*)album->extraData(this);

    if(!item)
        return;

    AlbumThumbnailLoader *loader = AlbumThumbnailLoader::componentData();
    QPixmap blendedIcon = loader->blendIcons(loader->getStandardTagIcon(), thumbnail);
    item->setPixmap(0, blendedIcon);
}

void TagFilterView::slotThumbnailLost(Album *)
{
    // we already set the standard icon before loading
}

void TagFilterView::slotReloadThumbnails()
{
    AlbumList tList = AlbumManager::instance()->allTAlbums();
    for (AlbumList::iterator it = tList.begin(); it != tList.end(); ++it)
    {
        TAlbum* tag  = (TAlbum*)(*it);
        setTagThumbnail(tag);
    }
}

void TagFilterView::slotAlbumIconChanged(Album* album)
{
    if(!album || album->type() != Album::TAG)
        return;

    setTagThumbnail((TAlbum *)album);
}

void TagFilterView::slotClear()
{
    clear();

    TagFilterViewItem* notTaggedItem = new TagFilterViewItem(this, 0, true);
    notTaggedItem->setPixmap(0, AlbumThumbnailLoader::componentData()->getStandardTagIcon());
}

void TagFilterView::slotTimeOut()
{
    Q3ValueList<int> filterTags;

    bool showUnTagged = false;

    Q3ListViewItemIterator it(this, Q3ListViewItemIterator::Checked);
    while (it.current())
    {
        TagFilterViewItem* item = (TagFilterViewItem*)it.current();
        if (item->m_tag)
            filterTags.append(item->m_tag->id());
        else if (item->m_untagged)
            showUnTagged = true;
        ++it;
    }

    AlbumLister::componentData()->setTagFilter(filterTags, d->matchingCond, showUnTagged);
}

void TagFilterView::slotContextMenu(Q3ListViewItem* it, const QPoint&, int)
{
    TagFilterViewItem *item = dynamic_cast<TagFilterViewItem*>(it);
    if (item && item->m_untagged)
        return;

    KMenu popmenu(this);
    popmenu.addTitle(SmallIcon("digikam"), i18n("Tag Filters"));

    QAction *newAction, *editAction=0, *resetIconAction=0, *deleteAction=0;

    newAction     = popmenu.addAction(SmallIcon("tag-new"), i18n("New Tag..."));

#ifdef KDEPIMLIBS_FOUND
    d->ABCMenu = new QMenu(this);

    connect( d->ABCMenu, SIGNAL( aboutToShow() ),
             this, SLOT( slotABCContextMenu() ) );

    popmenu.addMenu(d->ABCMenu);
    d->ABCMenu->menuAction()->setIcon(SmallIcon("tag-addressbook"));
    d->ABCMenu->menuAction()->setText(i18n("Create Tag From AddressBook"));
#endif

    if (item)
    {
        editAction      = popmenu.addAction(SmallIcon("tag-properties"), i18n("Edit Tag Properties..."));
        resetIconAction = popmenu.addAction(SmallIcon("tag-reset"),      i18n("Reset Tag Icon"));
        popmenu.addSeparator();
        deleteAction    = popmenu.addAction(SmallIcon("tag-delete"),     i18n("Delete Tag"));
    }

    popmenu.addSeparator();

    QMenu selectTagsMenu;
    QAction *selectAllTagsAction, *selectChildrenAction=0, *selectParentsAction=0;
    selectAllTagsAction = selectTagsMenu.addAction(i18n("All Tags"));
    if (item)
    {
        selectTagsMenu.addSeparator();
        selectChildrenAction = selectTagsMenu.addAction(i18n("Children"));
        selectParentsAction  = selectTagsMenu.addAction(i18n("Parents"));
    }
    popmenu.addMenu(&selectTagsMenu);
    selectTagsMenu.menuAction()->setText(i18n("Select"));

    QMenu deselectTagsMenu;
    QAction *deselectAllTagsAction, *deselectChildrenAction=0, *deselectParentsAction=0;
    deselectAllTagsAction = deselectTagsMenu.addAction(i18n("All Tags"));
    if (item)
    {
        deselectTagsMenu.addSeparator();
        deselectChildrenAction = deselectTagsMenu.addAction(i18n("Children"));
        deselectParentsAction  = deselectTagsMenu.addAction(i18n("Parents"));
    }
    popmenu.addMenu(&deselectTagsMenu);
    deselectTagsMenu.menuAction()->setText(i18n("Deselect"));

    QAction *invertAction;
    invertAction = popmenu.addAction(i18n("Invert Selection"));
    popmenu.addSeparator();


    KSelectAction *toggleAutoAction = new KSelectAction(i18n("Toogle Auto"), &popmenu);
    QAction *toggleNoneAction     = toggleAutoAction->addAction(i18n("None"));
    toggleAutoAction->menu()->addSeparator();
    QAction *toggleChildrenAction = toggleAutoAction->addAction(i18n("Children"));
    QAction *toggleParentsAction  = toggleAutoAction->addAction(i18n("Parents"));
    QAction *toggleBothAction     = toggleAutoAction->addAction(i18n("Both"));

    toggleNoneAction->setChecked(d->toggleAutoTags == TagFilterView::NoToggleAuto);
    toggleChildrenAction->setChecked(d->toggleAutoTags == TagFilterView::Children);
    toggleParentsAction->setChecked(d->toggleAutoTags == TagFilterView::Parents);
    toggleBothAction->setChecked(d->toggleAutoTags == TagFilterView::ChildrenAndParents);

    popmenu.addAction(toggleAutoAction);


    KSelectAction *matchingCondAction = new KSelectAction(i18n("Matching Condition"), &popmenu);
    QAction *orBetweenAction = matchingCondAction->addAction(i18n("Or Between Tags"));
    QAction *andBetweenAction = matchingCondAction->addAction(i18n("And Between Tags"));

    if (d->matchingCond == AlbumLister::OrCondition)
        orBetweenAction->setChecked(true);
    else
        andBetweenAction->setChecked(true);
    popmenu.addAction(matchingCondAction);

    ToggleAutoTags oldAutoTags = d->toggleAutoTags;

    QAction *choice = popmenu.exec((QCursor::pos()));

    if (choice)
    {
        if (choice == newAction)                    // New Tag.
        {
            tagNew(item);
        }
        else if (choice == editAction)              // Edit Tag Properties.
        {
            tagEdit(item);
        }
        else if (choice == deleteAction)            // Delete Tag.
        {
            tagDelete(item);
        }
        else if (choice == resetIconAction)         // Reset Tag Icon.
        {
            QString errMsg;
            AlbumManager::instance()->updateTAlbumIcon(item->m_tag, QString("tag"), 0, errMsg);
        }
        else if (choice == selectAllTagsAction)     // Select All Tags.
        {
            d->toggleAutoTags = TagFilterView::NoToggleAuto;
            Q3ListViewItemIterator it(this, Q3ListViewItemIterator::NotChecked);
            while (it.current())
            {
                TagFilterViewItem* item = (TagFilterViewItem*)it.current();

                // Ignore "Not Tagged" tag filter.
                if (!item->m_untagged)
                    item->setOn(true);

                ++it;
            }
            triggerChange();
            d->toggleAutoTags = oldAutoTags;
        }
        else if (choice == deselectAllTagsAction)    // Deselect All Tags.
        {
            d->toggleAutoTags = TagFilterView::NoToggleAuto;
            Q3ListViewItemIterator it(this, Q3ListViewItemIterator::Checked);
            while (it.current())
            {
                TagFilterViewItem* item = (TagFilterViewItem*)it.current();

                // Ignore "Not Tagged" tag filter.
                if (!item->m_untagged)
                    item->setOn(false);

                ++it;
            }
            triggerChange();
            d->toggleAutoTags = oldAutoTags;
        }
        else if (choice == invertAction)             // Invert All Tags Selection.
        {
            d->toggleAutoTags = TagFilterView::NoToggleAuto;
            Q3ListViewItemIterator it(this);
            while (it.current())
            {
                TagFilterViewItem* item = (TagFilterViewItem*)it.current();

                // Ignore "Not Tagged" tag filter.
                if (!item->m_untagged)
                    item->setOn(!item->isOn());

                ++it;
            }
            triggerChange();
            d->toggleAutoTags = oldAutoTags;
        }
        else if (choice == selectChildrenAction)     // Select Child Tags.
        {
            d->toggleAutoTags = TagFilterView::NoToggleAuto;
            toggleChildTags(item, true);
            TagFilterViewItem *tItem = (TagFilterViewItem*)item->m_tag->extraData(this);
            tItem->setOn(true);
            d->toggleAutoTags = oldAutoTags;
        }
        else if (choice == deselectChildrenAction)   // Deselect Child Tags.
        {
            d->toggleAutoTags = TagFilterView::NoToggleAuto;
            toggleChildTags(item, false);
            TagFilterViewItem *tItem = (TagFilterViewItem*)item->m_tag->extraData(this);
            tItem->setOn(false);
            d->toggleAutoTags = oldAutoTags;
        }
        else if (choice == selectParentsAction)     // Select Parent Tags.
        {
            d->toggleAutoTags = TagFilterView::NoToggleAuto;
            toggleParentTags(item, true);
            TagFilterViewItem *tItem = (TagFilterViewItem*)item->m_tag->extraData(this);
            tItem->setOn(true);
            d->toggleAutoTags = oldAutoTags;
        }
        else if (choice == deselectParentsAction)   // Deselect Parent Tags.
        {
            d->toggleAutoTags = TagFilterView::NoToggleAuto;
            toggleParentTags(item, false);
            TagFilterViewItem *tItem = (TagFilterViewItem*)item->m_tag->extraData(this);
            tItem->setOn(false);
            d->toggleAutoTags = oldAutoTags;
        }
        else if (choice == toggleNoneAction)        // No toggle auto tags.
        {
            d->toggleAutoTags = NoToggleAuto;
        }
        else if (choice == toggleChildrenAction)    // Toggle auto Children tags.
        {
            d->toggleAutoTags = Children;
        }
        else if (choice == toggleParentsAction)     // Toggle auto Parents tags.
        {
            d->toggleAutoTags = Parents;
        }
        else if (choice == toggleBothAction)        // Toggle auto Children and Parents tags.
        {
            d->toggleAutoTags = ChildrenAndParents;
        }
        else if (choice == orBetweenAction)         // Or Between Tags.
        {
            d->matchingCond = AlbumLister::OrCondition;
            triggerChange();
        }
        else if (choice == andBetweenAction)        // And Between Tags.
        {
            d->matchingCond = AlbumLister::AndCondition;
            triggerChange();
        }
        else                                        // ABC menu
        {
            tagNew(item, choice->text(), "tag-people" );
        }
    }

    delete d->ABCMenu;
    d->ABCMenu = 0;
}

void TagFilterView::slotABCContextMenu()
{
#ifdef KDEPIMLIBS_FOUND
    d->ABCMenu->clear();

    KABC::AddressBook* ab = KABC::StdAddressBook::self();
    QStringList names;
    for ( KABC::AddressBook::Iterator it = ab->begin(); it != ab->end(); ++it )
    {
        names.push_back(it->formattedName());
    }
    qSort(names);

    for ( QStringList::Iterator it = names.begin(); it != names.end(); ++it )
    {
        QString name = *it;
        if (!name.isNull() )
            d->ABCMenu->addAction(name);
    }

    if (d->ABCMenu->isEmpty())
    {
        QAction *nothingFound = d->ABCMenu->addAction(i18n("No AddressBook entries found"));
        nothingFound->setEnabled(false);
    }
#endif
}

void TagFilterView::tagNew(TagFilterViewItem* item, const QString& _title, const QString& _icon)
{
    TAlbum  *parent;
    QString  title    = _title;
    QString  icon     = _icon;
    AlbumManager *man = AlbumManager::instance();

    if (!item)
        parent = man->findTAlbum(0);
    else
        parent = item->m_tag;

    if (title.isNull())
    {
        if (!TagCreateDlg::tagCreate(kapp->activeWindow(), parent, title, icon))
            return;
    }

    QString errMsg;
    TAlbum* newAlbum = man->createTAlbum(parent, title, icon, errMsg);

    if( !newAlbum )
    {
        KMessageBox::error(0, errMsg);
    }
    else
    {
        TagFilterViewItem *item = (TagFilterViewItem*)newAlbum->extraData(this);
        if ( item )
        {
            clearSelection();
            setSelected(item, true);
            setCurrentItem(item);
            ensureItemVisible( item );
        }
    }
}

void TagFilterView::tagEdit(TagFilterViewItem* item)
{
    if (!item)
        return;

    TAlbum *tag = item->m_tag;
    if (!tag)
        return;

    QString title, icon;
    if (!TagEditDlg::tagEdit(kapp->activeWindow(), tag, title, icon))
    {
        return;
    }

    AlbumManager* man = AlbumManager::instance();

    if (tag->title() != title)
    {
        QString errMsg;
        if(!man->renameTAlbum(tag, title, errMsg))
            KMessageBox::error(0, errMsg);
        else
            item->setText(0, title);
    }

    if (tag->icon() != icon)
    {
        QString errMsg;
        if (!man->updateTAlbumIcon(tag, icon, 0, errMsg))
            KMessageBox::error(0, errMsg);
        else
            setTagThumbnail(tag);
    }
}

void TagFilterView::tagDelete(TagFilterViewItem* item)
{
    if (!item)
        return;

    TAlbum *tag = item->m_tag;
    if (!tag || tag->isRoot())
        return;

    // find number of subtags
    int children = 0;
    AlbumIterator iter(tag);
    while(iter.current())
    {
        children++;
        ++iter;
    }

    AlbumManager* man = AlbumManager::instance();

    if (children)
    {
        int result = KMessageBox::warningContinueCancel(this,
                     i18np("Tag '%1' has one subtag. "
                           "Deleting this will also delete "
                           "the subtag. "
                           "Do you want to continue?",
                           "Tag '%1' has %n subtags. "
                           "Deleting this will also delete "
                           "the subtags. "
                           "Do you want to continue?",
                           children, tag->title()),
                     i18n("Delete Tag"),
                     KGuiItem(i18n("Delete"),
                     "edit-delete"));

        if(result == KMessageBox::Continue)
        {
            QString errMsg;
            if (!man->deleteTAlbum(tag, errMsg))
                KMessageBox::error(0, errMsg);
        }
    }
    else
    {
        int result = KMessageBox::warningContinueCancel(0, i18n("Delete '%1' tag?", tag->title()),
                                                        i18n("Delete Tag"),
                                                        KGuiItem(i18n("Delete"), "edit-delete"));

        if (result == KMessageBox::Continue)
        {
            QString errMsg;
            if (!man->deleteTAlbum(tag, errMsg))
                KMessageBox::error(0, errMsg);
        }
    }
}

void TagFilterView::toggleChildTags(TagFilterViewItem* tItem, bool b)
{
    if (!tItem)
        return;

    TAlbum *album = tItem->m_tag; 
    if (!album)
        return;

    AlbumIterator it(album);
    while ( it.current() )
    {
        TAlbum *ta              = (TAlbum*)it.current();
        TagFilterViewItem *item = (TagFilterViewItem*)ta->extraData(this);
        if (item)
            if (item->isVisible())
                item->setOn(b);
        ++it;
    }
}

void TagFilterView::toggleParentTags(TagFilterViewItem* tItem, bool b)
{
    if (!tItem)
        return;

    TAlbum *album = tItem->m_tag; 
    if (!album)
        return;

    Q3ListViewItemIterator it(this);
    while (it.current())
    {
        TagFilterViewItem* item = dynamic_cast<TagFilterViewItem*>(it.current());
        if (item->isVisible())
        {
            Album *a = dynamic_cast<Album*>(item->m_tag);
            if (a)
            {
                if (a == album->parent())
                {
                    item->setOn(b);
                    toggleParentTags(item, b);
                }
            }
        }
        ++it;
    }
}

}  // namespace Digikam

