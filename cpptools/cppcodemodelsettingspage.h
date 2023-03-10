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

#ifndef CPPTOOLS_INTERNAL_CPPCODEMODELSETTINGSPAGE_H
#define CPPTOOLS_INTERNAL_CPPCODEMODELSETTINGSPAGE_H

#include "cppcodemodelsettings.h"

#include <core/dialogs/ioptionspage.h>

#include <QPointer>
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QSettings)

namespace CppTools {
namespace Internal {

namespace Ui { class CppCodeModelSettingsPage; }

class CppCodeModelSettingsWidget: public QWidget
{
    Q_OBJECT

public:
    explicit CppCodeModelSettingsWidget(QWidget *parent = 0);
    ~CppCodeModelSettingsWidget();

    void setSettings(const QSharedPointer<CppCodeModelSettings> &s);
    void applyToSettings() const;

private:
    void setupClangCodeModelWidgets() const;
    void setupPchCheckBox() const;

    bool applyClangCodeModelWidgetsToSettings() const;
    bool applyPchCheckBoxToSettings() const;

private:
    Ui::CppCodeModelSettingsPage *m_ui;
    QSharedPointer<CppCodeModelSettings> m_settings;
};

class CppCodeModelSettingsPage: public Core::IOptionsPage
{
public:
    explicit CppCodeModelSettingsPage(QSharedPointer<CppCodeModelSettings> &settings,
                                      QObject *parent = 0);

    QWidget *widget();
    void apply();
    void finish();

private:
    const QSharedPointer<CppCodeModelSettings> m_settings;
    QPointer<CppCodeModelSettingsWidget> m_widget;
};

} // Internal namespace
} // CppTools namespace

#endif // CPPTOOLS_INTERNAL_CPPCODEMODELSETTINGSPAGE_H
