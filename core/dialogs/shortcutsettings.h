/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2022 Rochus Keller (me@rochus-keller.ch) for LeanCreator
**
** This file is part of Qt Creator.
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

#ifndef SHORTCUTSETTINGS_H
#define SHORTCUTSETTINGS_H

#include <core/actionmanager/commandmappings.h>
#include <core/dialogs/ioptionspage.h>

#include <QKeySequence>
#include <QPointer>
#include <QPushButton>

#include <array>

QT_BEGIN_NAMESPACE
class QGroupBox;
class QKeyEvent;
class QLabel;
QT_END_NAMESPACE

namespace Core {

class Command;

namespace Internal {

class ActionManagerPrivate;
class MainWindow;

struct ShortcutItem
{
    Command *m_cmd;
    QKeySequence m_key;
    QTreeWidgetItem *m_item;
};

class ShortcutButton : public QPushButton
{
    Q_OBJECT
public:
    ShortcutButton(QWidget *parent = 0);

    QSize sizeHint() const;

signals:
    void keySequenceChanged(const QKeySequence &sequence);

protected:
    bool eventFilter(QObject *obj, QEvent *evt);

private:
    void updateText();
    void handleToggleChange(bool toggleState);

    QString m_checkedText;
    QString m_uncheckedText;
    mutable int m_preferredWidth = -1;
    std::array<int, 4> m_key;
    int m_keyNum = 0;
};

class ShortcutSettingsWidget : public CommandMappings
{
    Q_OBJECT

public:
    ShortcutSettingsWidget(QWidget *parent = 0);
    ~ShortcutSettingsWidget() override;

    void apply();

protected:
    void importAction() override;
    void exportAction() override;
    void defaultAction() override;
    bool filterColumn(const QString &filterString, QTreeWidgetItem *item, int column) const override;

private:
    void initialize();
    void handleCurrentCommandChanged(QTreeWidgetItem *current);
    void resetToDefault();
    bool validateShortcutEdit() const;
    bool markCollisions(ShortcutItem *);
    void setKeySequence(const QKeySequence &key);
    void showConflicts();
    void clear();

    QList<ShortcutItem *> m_scitems;
    QGroupBox *m_shortcutBox;
    Utils::FancyLineEdit *m_shortcutEdit;
    QLabel *m_warningLabel;
};

class ShortcutSettings : public IOptionsPage
{
    Q_OBJECT

public:
    ShortcutSettings(QObject *parent = 0);

    QWidget *widget() override;
    void apply() override;
    void finish() override;

private:
    QPointer<ShortcutSettingsWidget> m_widget;
};

} // namespace Internal
} // namespace Core

#endif // SHORTCUTSETTINGS_H
