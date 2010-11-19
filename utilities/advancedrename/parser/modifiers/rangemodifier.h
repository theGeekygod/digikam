/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-09-18
 * Description : a modifier for displaying only a range of a token result
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

#ifndef RANGEMODIFIER_H
#define RANGEMODIFIER_H

// Local includes

#include "modifier.h"
#include "parseabledialog.h"

namespace Ui
{
class RangeModifierDialogWidget;
}

namespace Digikam
{

class RangeDialog : public ParseableDialog
{
    Q_OBJECT

public:

    RangeDialog(Parseable* parent);
    ~RangeDialog();

    Ui::RangeModifierDialogWidget* const ui;

private Q_SLOTS:

    void slotToTheEndChecked(bool checked);
};

// --------------------------------------------------------

class RangeModifier : public Modifier
{
    Q_OBJECT

public:

    RangeModifier();
    virtual QString parseOperation(ParseSettings& settings);

private Q_SLOTS:

    void slotTokenTriggered(const QString& token);
};

} // namespace Digikam


#endif /* RANGEMODIFIER_H */
