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

#ifndef CUSTOMEXECUTABLECONFIGURATIONWIDGET_H
#define CUSTOMEXECUTABLECONFIGURATIONWIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QComboBox;
class QLabel;
class QAbstractButton;
QT_END_NAMESPACE

namespace Utils {
class DetailsWidget;
class PathChooser;
}

namespace ProjectExplorer {
class ArgumentsAspect;
class TerminalAspect;
}

namespace GenericProjectManager {
class CustomExecutableRunConfiguration;

namespace Internal {

class CustomExecutableConfigurationWidget : public QWidget
{
    Q_OBJECT

public:
    enum ApplyMode { InstantApply, DelayedApply};
    CustomExecutableConfigurationWidget(CustomExecutableRunConfiguration *rc, ApplyMode mode);
    ~CustomExecutableConfigurationWidget();

    void apply(); // only used for DelayedApply

    bool isValid() const;
signals:
    void validChanged();

private slots:
    void changed();

    void executableEdited();
    void workingDirectoryEdited();
    void environmentWasChanged();

private:
    bool m_ignoreChange;
    CustomExecutableRunConfiguration *m_runConfiguration;
    ProjectExplorer::ArgumentsAspect *m_temporaryArgumentsAspect;
    ProjectExplorer::TerminalAspect *m_temporaryTerminalAspect;
    Utils::PathChooser *m_executableChooser;
    Utils::PathChooser *m_workingDirectory;
    Utils::DetailsWidget *m_detailsContainer;
};

} // namespace Internal
} // namespace QtSupport

#endif // CUSTOMEXECUTABLECONFIGURATIONWIDGET_H
