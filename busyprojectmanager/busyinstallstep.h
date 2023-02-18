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

#ifndef BUSYINSTALLSTEP_H
#define BUSYINSTALLSTEP_H

#include "busybuildconfiguration.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/task.h>

#include <busytools/busyapi.h>

namespace BusyProjectManager {
namespace Internal {

class BusyInstallStepConfigWidget;

class BusyInstallStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    BusyInstallStep(ProjectExplorer::BuildStepList *bsl);
    BusyInstallStep(ProjectExplorer::BuildStepList *bsl, const BusyInstallStep *other);
    ~BusyInstallStep();

    bool init();

    void run(QFutureInterface<bool> &fi);

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();

    bool runInGuiThread() const;
    void cancel();

    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

    busy::InstallOptions installOptions() const;
    QString installRoot() const;
    QString absoluteInstallRoot() const;
    bool removeFirst() const;
    bool dryRun() const;
    bool keepGoing() const;

signals:
    void changed();

private slots:
    void installDone(bool success);
    void handleTaskStarted(const QString &desciption, int max);
    void handleProgress(int value);

private:
    void createTaskAndOutput(ProjectExplorer::Task::TaskType type,
                             const QString &message, const QString &file, int line);

    void setInstallRoot(const QString &ir);
    void setRemoveFirst(bool rf);
    void setDryRun(bool dr);
    void setKeepGoing(bool kg);

    busy::InstallOptions m_qbsInstallOptions;

    QFutureInterface<bool> *m_fi;
    busy::InstallJob *m_job;
    int m_progressBase;
    bool m_showCompilerOutput;
    ProjectExplorer::IOutputParser *m_parser;

    friend class BusyInstallStepConfigWidget;
};

namespace Ui { class BusyInstallStepConfigWidget; }

class BusyInstallStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    BusyInstallStepConfigWidget(BusyInstallStep *step);
    ~BusyInstallStepConfigWidget();
    QString summaryText() const;
    QString displayName() const;

private slots:
    void updateState();

    void changeInstallRoot();
    void changeRemoveFirst(bool rf);
    void changeDryRun(bool dr);
    void changeKeepGoing(bool kg);

private:
    Ui::BusyInstallStepConfigWidget *m_ui;

    BusyInstallStep *m_step;
    QString m_summary;
    bool m_ignoreChange;
};


class BusyInstallStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit BusyInstallStepFactory(QObject *parent = 0);

    // used to show the list of possible additons to a target, returns a list of types
    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const;
    // used to translate the types to names to display to the user
    QString displayNameForId(Core::Id id) const;

    bool canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id);
    // used to recreate the runConfigurations when restoring settings
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);
    bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product);
};


} // namespace Internal
} // namespace BusyProjectManager

#endif // BUSYINSTALLSTEP_H
