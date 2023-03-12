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

#include "wizardsplugin.h"
#include "wizardsconstants.h"
#include "classwizard.h"
#include "qrcwizard.h"
#include "busyprowizard.h"

#include <core/icore.h>
#include <core/icontext.h>
#include <core/actionmanager/actionmanager.h>
#include <core/actionmanager/command.h>
#include <core/actionmanager/actioncontainer.h>
#include <core/coreconstants.h>

#include <QAction>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>
#include <QtPlugin>
using namespace Wizards::Internal;

WizardsPlugin::WizardsPlugin()
{
    // Create your members
}

WizardsPlugin::~WizardsPlugin()
{
    // Unregister objects from the plugin manager's object pool
    // Delete members
}

bool WizardsPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    // Register objects in the plugin manager's object pool
    // Load settings
    // Add actions to menus
    // Connect to other plugins' signals
    // In the initialize function, a plugin can be sure that the plugins it
    // depends on have initialized their members.

    Q_UNUSED(arguments)
    Q_UNUSED(errorString)


    Core::IWizardFactory::registerFactoryCreator(
                []() {
                        QList<Core::IWizardFactory *> res;
                        res.append( new ClassWizardFactory() );
                        res.append( new QrcWizardFactory() );
                        res.append( new BusyProWizardFactory() );
                        return res;
                     });


    return true;
}

void WizardsPlugin::extensionsInitialized()
{
    // Retrieve objects from the plugin manager's object pool
    // In the extensionsInitialized function, a plugin can be sure that all
    // plugins that depend on it are completely initialized.
}

ExtensionSystem::IPlugin::ShutdownFlag WizardsPlugin::aboutToShutdown()
{
    // Save settings
    // Disconnect from signals that are not needed during shutdown
    // Hide UI (if you add UI that is not in the main window directly)
    return SynchronousShutdown;
}

