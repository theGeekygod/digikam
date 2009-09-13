/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-05-22
 * Description : a control widget for the ManualRenameParser
 *
 * Copyright (C) 2009 by Andi Clemens <andi dot clemens at gmx dot net>
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

#include "manualrenamewidget.h"
#include "manualrenamewidget.moc"

// Qt includes

#include <QAction>
#include <QDateTime>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QList>
#include <QMenu>
#include <QPushButton>
#include <QTimer>
#include <QToolButton>

// KDE includes

#include <kicon.h>
#include <kiconloader.h>
#include <klocale.h>
#include <knuminput.h>

// Local includes

#include "parser.h"
#include "dcursortracker.h"
#include "defaultparser.h"
#include "manualrenameinput.h"

namespace Digikam
{

class ManualRenameWidgetPriv
{
public:

    ManualRenameWidgetPriv()
    {
        parserLineEdit        = 0;
        tooltipTracker        = 0;
        tooltipToggleButton   = 0;
        insertTokenToolButton = 0;
        btnContainer          = 0;
        parser                = 0;
        inputColumns          = 2;
        inputStyles           = ManualRenameWidget::BigButtons;
    }

    int                             inputColumns;
    ManualRenameWidget::InputStyles inputStyles;

    QToolButton*                    tooltipToggleButton;
    QToolButton*                    insertTokenToolButton;
    QGroupBox*                      btnContainer;

    DTipTracker*                    tooltipTracker;
    ManualRenameInput*              parserLineEdit;
    Parser*                         parser;
};

ManualRenameWidget::ManualRenameWidget(QWidget* parent)
                 : QWidget(parent), d(new ManualRenameWidgetPriv)
{
    setupWidgets();
    setParser(new DefaultParser());
}

ManualRenameWidget::~ManualRenameWidget()
{
    // we need to delete it manually, because it has no parent
    delete d->tooltipTracker;

    delete d->parser;
    delete d;
}

QString ManualRenameWidget::text() const
{
    return d->parserLineEdit->input()->text();
}

void ManualRenameWidget::setText(const QString& text)
{
    d->parserLineEdit->input()->setText(text);
}

void ManualRenameWidget::setTrackerAlignment(Qt::Alignment alignment)
{
    d->tooltipTracker->setTrackerAlignment(alignment);
}

void ManualRenameWidget::clear()
{
    d->parserLineEdit->input()->clear();
}

void ManualRenameWidget::slotHideToolTipTracker()
{
    d->tooltipToggleButton->setChecked(false);
    slotToolTipButtonToggled(false);
}

QString ManualRenameWidget::parse(ParseInformation& info) const
{
    if (!d->parser)
        return QString();

    QString parseString = d->parserLineEdit->input()->text();

    QString parsed;
    parsed = d->parser->parse(parseString, info);

    return parsed;
}

void ManualRenameWidget::createToolTip()
{
    if (!d->parser)
        d->tooltipTracker->setText(QString());

    QString tooltip;
    tooltip += QString("<p><table>");

    foreach (SubParser* subparser, d->parser->subParsers())
    {
        foreach (Token* token, subparser->tokens())
        {
            tooltip += QString("<tr><td><b>%1</b></td><td>:</td><td>%2</td></tr>")
                    .arg(token->id())
                    .arg(token->description());
        }
    }

    tooltip += QString("</table></p>");
    tooltip += d->parserLineEdit->input()->toolTip();

    d->tooltipTracker->setText(tooltip);
}

void ManualRenameWidget::slotToolTipButtonToggled(bool checked)
{
    d->tooltipTracker->setVisible(checked);
    slotUpdateTrackerPos();
}

void ManualRenameWidget::slotUpdateTrackerPos()
{
    d->tooltipTracker->refresh();
}

void ManualRenameWidget::setInputStyle(InputStyles inputMask)
{
    bool enable = d->parser && d->parser->subParsers().count() > 0;

    d->btnContainer->setVisible(enable && (inputMask & BigButtons));
    d->insertTokenToolButton->setVisible(enable && (inputMask & ToolButton));
    d->parserLineEdit->setEnabled(enable);
    d->tooltipToggleButton->setVisible(enable);

    d->inputStyles = inputMask;
}

void ManualRenameWidget::registerParsers()
{
   if (d->parser)
   {
       setupWidgets();

       QMenu* tokenToolBtnMenu = new QMenu(d->insertTokenToolButton);
       int column              = 0;
       int row                 = 0;
       QPushButton* btn        = 0;
       QAction* action         = 0;
       QGridLayout* gridLayout = new QGridLayout;

       int maxParsers = d->parser->subParsers().count();
       foreach (SubParser* subparser, d->parser->subParsers())
       {
           btn    = subparser->registerButton(this);
           action = subparser->registerMenu(tokenToolBtnMenu);

           if (!btn || !action)
               continue;

           gridLayout->addWidget(btn, row, column, 1, 1);

           connect(subparser, SIGNAL(signalTokenTriggered(const QString&)),
                   d->parserLineEdit, SLOT(slotAddToken(const QString&)));

           ++column;

           if (column % d->inputColumns == 0)
           {
               ++row;
               column = 0;
           }
       }

       // --------------------------------------------------------

       // If the buttons don't fill up all columns, expand the last button to fit the layout
       if ((row >= (maxParsers / d->inputColumns)) && (column == 0))
       {
           gridLayout->removeWidget(btn);
           gridLayout->addWidget(btn, (row - 1), (d->inputColumns - 1), 1, 1);
       }
       else if (column != d->inputColumns)
       {
           gridLayout->removeWidget(btn);
           gridLayout->addWidget(btn, row, (column - 1), 1, -1);
       }

       // --------------------------------------------------------

       d->btnContainer->setLayout(gridLayout);
       d->insertTokenToolButton->setMenu(tokenToolBtnMenu);

       d->parserLineEdit->input()->setParser(d->parser);
       createToolTip();
   }
}

void ManualRenameWidget::setParser(Parser* parser)
{
    if (!parser)
        return;

    if (d->parser)
        delete d->parser;
    d->parser = parser;

    setInputColumns(d->inputColumns);
    registerParsers();
    setInputStyle(d->inputStyles);
}

void ManualRenameWidget::setInputColumns(int col)
{
    if (col == d->inputColumns)
        return;

    d->inputColumns = col;
    registerParsers();
    setInputStyle(d->inputStyles);
}

void ManualRenameWidget::setupWidgets()
{
    delete d->parserLineEdit;
    d->parserLineEdit = new ManualRenameInput;

    delete d->tooltipToggleButton;
    d->tooltipToggleButton = new QToolButton;
    d->tooltipToggleButton->setCheckable(true);
    d->tooltipToggleButton->setIcon(SmallIcon("dialog-information"));

    // --------------------------------------------------------

    delete d->btnContainer;
    d->btnContainer = new QGroupBox(i18n("Renaming Options"), this);

    delete d->insertTokenToolButton;
    d->insertTokenToolButton = new QToolButton;
    d->insertTokenToolButton->setPopupMode(QToolButton::InstantPopup);
    d->insertTokenToolButton->setIcon(SmallIcon("list-add"));

    // --------------------------------------------------------

    delete d->tooltipTracker;
    d->tooltipTracker = new DTipTracker(QString(), d->parserLineEdit, Qt::AlignLeft);
    d->tooltipTracker->setEnable(false);
    d->tooltipTracker->setKeepOpen(true);
    d->tooltipTracker->setOpenExternalLinks(true);
    setTrackerAlignment(Qt::AlignLeft);

    // --------------------------------------------------------

    delete layout();
    QGridLayout* mainLayout = new QGridLayout;
    mainLayout->addWidget(d->parserLineEdit,        0, 0, 1, 1);
    mainLayout->addWidget(d->tooltipToggleButton,   0, 1, 1, 1);
    mainLayout->addWidget(d->insertTokenToolButton, 0, 2, 1, 1);
    mainLayout->addWidget(d->btnContainer,          1, 0, 1,-1);
    mainLayout->setColumnStretch(0, 10);
    setLayout(mainLayout);

    connect(d->tooltipToggleButton, SIGNAL(toggled(bool)),
            this, SLOT(slotToolTipButtonToggled(bool)));

    connect(d->parserLineEdit, SIGNAL(signalTextChanged(const QString&)),
            this, SIGNAL(signalTextChanged(const QString&)));
}

}  // namespace Digikam
