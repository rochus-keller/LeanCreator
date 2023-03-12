#ifndef WIZARDS_BUSIPROWIZARD_H
#define WIZARDS_BUSIPROWIZARD_H

/****************************************************************************
**
** Copyright (C) 2023 Rochus Keller (me@rochus-keller.ch) for LeanCreator
**
** This file is part of LeanCreator.
**
** $QT_BEGIN_LICENSE:LGPL21$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <core/basefilewizard.h>
#include <core/basefilewizardfactory.h>
#include <utils/wizard.h>
#include <wizards/wizards_global.h>

namespace Utils { class FileWizardPage; }

namespace Wizards {

class BusyProWizardDialog : public Core::BaseFileWizard
{
    Q_OBJECT

public:
    BusyProWizardDialog(const Core::BaseFileWizardFactory *factory, QWidget *parent = 0);

    Utils::FileWizardPage* m_firstPage;
};

class BusyProWizardFactory : public Core::BaseFileWizardFactory
{
    Q_OBJECT

public:
    BusyProWizardFactory();

protected:
    Core::BaseFileWizard *create(QWidget *parent, const Core::WizardDialogParameters &parameters) const;
    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const;
    bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage) const;
};


}

#endif // WIZARDS_BUSIPROWIZARD_H
