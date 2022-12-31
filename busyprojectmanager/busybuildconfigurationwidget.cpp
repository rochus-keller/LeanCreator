/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2022 Rochus Keller (me@rochus-keller.ch) for LeanCreator
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

#include "busybuildconfigurationwidget.h"

#include "busybuildconfiguration.h"

#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <utils/detailswidget.h>
#include <utils/pathchooser.h>

#include <QGridLayout>
#include <QLabel>
#include <QWidget>

namespace BusyProjectManager {
namespace Internal {

BusyBuildConfigurationWidget::BusyBuildConfigurationWidget(BusyProjectManager::Internal::BusyBuildConfiguration *bc) :
    m_buildConfiguration(bc),
    m_ignoreChange(false)
{
    connect(bc, SIGNAL(buildDirectoryChanged()), this, SLOT(buildDirectoryChanged()));
    connect(bc, SIGNAL(environmentChanged()), this, SLOT(environmentHasChanged()));

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);
    Utils::DetailsWidget *container = new Utils::DetailsWidget(this);
    container->setState(Utils::DetailsWidget::NoSummary);
    vbox->addWidget(container);

    QWidget *details = new QWidget(container);
    container->setWidget(details);

    QGridLayout *layout = new QGridLayout(details);
    layout->setMargin(0);
    layout->addWidget(new QLabel(tr("Build directory:"), 0, 0));

    m_buildDirChooser = new Utils::PathChooser;
    m_buildDirChooser->setExpectedKind(Utils::PathChooser::Directory);
    m_buildDirChooser->setBaseFileName(bc->target()->project()->projectDirectory());
    m_buildDirChooser->setEnvironment(bc->environment());
    layout->addWidget(m_buildDirChooser, 0, 1);

    connect(m_buildDirChooser, SIGNAL(rawPathChanged(QString)), this, SLOT(buildDirEdited()));

    buildDirectoryChanged();
}

void BusyBuildConfigurationWidget::buildDirEdited()
{
    m_ignoreChange = true;
    m_buildConfiguration->setBuildDirectory(m_buildDirChooser->fileName());
}

void BusyBuildConfigurationWidget::buildDirectoryChanged()
{
    if (m_ignoreChange)
        return;

    m_buildDirChooser->setPath(m_buildConfiguration->rawBuildDirectory().toString());
}

void BusyBuildConfigurationWidget::environmentHasChanged()
{
    m_buildDirChooser->setEnvironment(m_buildConfiguration->environment());
}

} // namespace Internal
} // namespace BusyProjectManager
