/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2008-08-20
 * Description : Image editor tool interface.
 *
 * Copyright (C) 2008 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#include <QWidget>

// Local includes.

#include "sidebar.h"
#include "editortool.h"
#include "editorstackview.h"
#include "editorwindow.h"
#include "editortooliface.h"
#include "editortooliface.moc"

namespace Digikam
{

class EditorToolIfacePriv
{

public:

    EditorToolIfacePriv()
    {
        prevTab = 0;
        editor  = 0;
        tool    = 0;
    }

    QWidget      *prevTab;

    EditorTool   *tool;

    EditorWindow *editor;
};

EditorToolIface* EditorToolIface::m_iface = 0;

EditorToolIface* EditorToolIface::editorToolIface()
{
    return m_iface;
}

EditorToolIface::EditorToolIface(EditorWindow *editor)
               : QObject()
{
    d = new EditorToolIfacePriv;
    d->editor = editor;
    m_iface   = this;
}

EditorToolIface::~EditorToolIface()
{
    delete d;
    if (m_iface == this)
        m_iface = 0;
}

EditorTool* EditorToolIface::currentTool() const
{
    return d->tool;
}

void EditorToolIface::loadTool(EditorTool* tool)
{
    if (d->tool) unLoadTool();

    d->tool = tool;
    d->editor->editorStackView()->setToolView(d->tool->toolView());
    d->editor->editorStackView()->setViewMode(EditorStackView::ToolViewMode);
    d->prevTab = d->editor->rightSideBar()->getActiveTab();
    d->editor->rightSideBar()->appendTab(d->tool->toolSettings(), d->tool->toolIcon(), d->tool->toolName());
    d->editor->rightSideBar()->setActiveTab(d->tool->toolSettings());
}

void EditorToolIface::unLoadTool()
{
    if (!d->tool) return;

    d->editor->editorStackView()->setViewMode(EditorStackView::CanvasMode);
    d->editor->editorStackView()->setToolView(0);
    d->editor->rightSideBar()->deleteTab(d->tool->toolSettings());
    d->editor->rightSideBar()->setActiveTab(d->prevTab);
    delete d->tool;
    d->tool = 0;
}

}  // namespace Digikam
