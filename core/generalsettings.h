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

#ifndef GENERALSETTINGS_H
#define GENERALSETTINGS_H

#include <core/dialogs/ioptionspage.h>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QMessageBox;
QT_END_NAMESPACE

namespace Core {
namespace Internal {

namespace Ui { class GeneralSettings; }

class GeneralSettings : public IOptionsPage
{
    Q_OBJECT

public:
    GeneralSettings();

    QWidget *widget();
    void apply();
    void finish();

private slots:
    void resetInterfaceColor();
    void resetWarnings();
    void resetLanguage();

private:
    bool canResetWarnings() const;
    void fillLanguageBox() const;
    QString language() const;
    void setLanguage(const QString&);
    Ui::GeneralSettings *m_page;
    QPointer<QMessageBox> m_dialog;
    QPointer<QWidget> m_widget;
};

} // namespace Internal
} // namespace Core

#endif // GENERALSETTINGS_H
