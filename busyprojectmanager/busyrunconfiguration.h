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

#ifndef BUSYRUNCONFIGURATION_H
#define BUSYRUNCONFIGURATION_H

#include <projectexplorer/localapplicationrunconfiguration.h>

#include <QStringList>
#include <QLabel>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QRadioButton;
class QComboBox;
QT_END_NAMESPACE

namespace busy { class InstallOptions; }

namespace Utils {
class PathChooser;
class DetailsWidget;
}

namespace ProjectExplorer { class BuildStepList; }

namespace BusyProjectManager {

class BusyProject;

namespace Internal {

class BusyInstallStep;
class BusyRunConfigurationFactory;

class BusyRunConfiguration : public ProjectExplorer::LocalApplicationRunConfiguration
{
    Q_OBJECT

    // to change the display name and arguments and set the userenvironmentchanges
    friend class BusyRunConfigurationWidget;
    friend class BusyRunConfigurationFactory;

public:
    BusyRunConfiguration(ProjectExplorer::Target *parent, Core::Id id);

    bool isEnabled() const;
    QString disabledReason() const;
    QWidget *createConfigurationWidget();

    QString executable() const;
    ProjectExplorer::ApplicationLauncher::Mode runMode() const;
    QString workingDirectory() const;
    QString commandLineArguments() const;

    Utils::OutputFormatter *createOutputFormatter() const;

    void setRunMode(ProjectExplorer::ApplicationLauncher::Mode runMode);

    void addToBaseEnvironment(Utils::Environment &env) const;

    QString uniqueProductName() const;
    bool isConsoleApplication() const;

signals:
    void targetInformationChanged();
    void usingDyldImageSuffixChanged(bool);

protected:
    BusyRunConfiguration(ProjectExplorer::Target *parent, BusyRunConfiguration *source);

private slots:
    void installStepChanged();
    void installStepToBeRemoved(int pos);

private:
    QString baseWorkingDirectory() const;
    QString defaultDisplayName();
    busy::InstallOptions installOptions() const;
    QString installRoot() const;

    void ctor();

    void updateTarget();

    QString m_uniqueProductName;

    // Cached startup sub project information

    BusyInstallStep *m_currentInstallStep; // We do not take ownership!
    ProjectExplorer::BuildStepList *m_currentBuildStepList; // We do not take ownership!
};

class BusyRunConfigurationWidget : public QWidget
{
    Q_OBJECT

public:
    BusyRunConfigurationWidget(BusyRunConfiguration *rc, QWidget *parent);

private slots:
    void runConfigurationEnabledChange();
    void targetInformationHasChanged();

private:
    void setExecutableLineText(const QString &text = QString());

    BusyRunConfiguration *m_rc;
    bool m_ignoreChange;
    QLabel *m_disabledIcon;
    QLabel *m_disabledReason;
    QLabel *m_executableLineLabel;
    QCheckBox *m_usingDyldImageSuffix;
    QLineEdit *m_qmlDebugPort;
    Utils::DetailsWidget *m_detailsContainer;
    bool m_isShown;
};

class BusyRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit BusyRunConfigurationFactory(QObject *parent = 0);
    ~BusyRunConfigurationFactory();

    bool canCreate(ProjectExplorer::Target *parent, Core::Id id) const;
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source);

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent, CreationMode mode) const;
    QString displayNameForId(Core::Id id) const;

private:
    bool canHandle(ProjectExplorer::Target *t) const;

    ProjectExplorer::RunConfiguration *doCreate(ProjectExplorer::Target *parent, Core::Id id);
    ProjectExplorer::RunConfiguration *doRestore(ProjectExplorer::Target *parent,
                                                 const QVariantMap &map);
};

} // namespace Internal
} // namespace BusyProjectManager

#endif // BUSYRUNCONFIGURATION_H
