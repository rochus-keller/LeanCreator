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

#ifndef CUSTOMEXECUTABLERUNCONFIGURATION_H
#define CUSTOMEXECUTABLERUNCONFIGURATION_H

#include <projectexplorer/localapplicationrunconfiguration.h>

#include <QVariantMap>

namespace ProjectExplorer { class Target; }

namespace GenericProjectManager {
namespace Internal { class CustomExecutableConfigurationWidget; }

class CustomExecutableRunConfigurationFactory;

class CustomExecutableRunConfiguration : public ProjectExplorer::LocalApplicationRunConfiguration
{
    Q_OBJECT
    // the configuration widget needs to setExecutable setWorkingDirectory and setCommandLineArguments
    friend class Internal::CustomExecutableConfigurationWidget;
    friend class CustomExecutableRunConfigurationFactory;

public:
    explicit CustomExecutableRunConfiguration(ProjectExplorer::Target *parent);
    ~CustomExecutableRunConfiguration();

    /**
     * Returns the executable, looks in the environment for it and might even
     * ask the user if none is specified
     */
    QString executable() const;

    /** Returns whether this runconfiguration ever was configured with an executable
     */
    bool isConfigured() const;

    ProjectExplorer::ApplicationLauncher::Mode runMode() const;
    QString workingDirectory() const;
    QString commandLineArguments() const;

    QWidget *createConfigurationWidget();

    ProjectExplorer::Abi abi() const;

    QVariantMap toMap() const;

    ConfigurationState ensureConfigured(QString *errorMessage);

signals:
    void changed();

protected:
    CustomExecutableRunConfiguration(ProjectExplorer::Target *parent,
                                     CustomExecutableRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);
    QString defaultDisplayName() const;

private slots:
    void configurationDialogFinished();
private:
    void ctor();

    void setExecutable(const QString &executable);
    QString rawExecutable() const;
    void setCommandLineArguments(const QString &commandLineArguments);
    void setBaseWorkingDirectory(const QString &workingDirectory);
    QString baseWorkingDirectory() const;
    void setUserName(const QString &name);
    void setRunMode(ProjectExplorer::ApplicationLauncher::Mode runMode);
    bool validateExecutable(QString *executable = 0, QString *errorMessage = 0) const;

    QString m_executable;
    QString m_workingDirectory;
    ProjectExplorer::ApplicationLauncher::Mode m_runMode;
    QWidget *m_dialog;
};

class CustomExecutableRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit CustomExecutableRunConfigurationFactory(QObject *parent = 0);
    ~CustomExecutableRunConfigurationFactory();

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent, CreationMode mode) const;
    QString displayNameForId(Core::Id id) const;

    bool canCreate(ProjectExplorer::Target *parent, Core::Id id) const;
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *product) const;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent,
                                             ProjectExplorer::RunConfiguration *source);

private:
    bool canHandle(ProjectExplorer::Target *parent) const;

    ProjectExplorer::RunConfiguration *doCreate(ProjectExplorer::Target *parent, Core::Id id);
    ProjectExplorer::RunConfiguration *doRestore(ProjectExplorer::Target *parent,
                                                 const QVariantMap &map);
};

} // namespace QtSupport

#endif // CUSTOMEXECUTABLERUNCONFIGURATION_H
