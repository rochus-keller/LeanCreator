/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of LeanCreator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEVICESETTINGSWIDGET_H
#define DEVICESETTINGSWIDGET_H

#include "idevice.h"

#include <QList>
#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer {
class IDevice;
class DeviceManager;
class DeviceManagerModel;
class IDeviceWidget;

namespace Internal {
namespace Ui { class DeviceSettingsWidget; }
class NameValidator;

class DeviceSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    DeviceSettingsWidget(QWidget *parent = 0);
    ~DeviceSettingsWidget();

    void saveSettings();

private slots:
    void handleDeviceUpdated(Core::Id id);
    void currentDeviceChanged(int index);
    void addDevice();
    void removeDevice();
    void deviceNameEditingFinished();
    void setDefaultDevice();
    void testDevice();
    void handleProcessListRequested();

private:
    void initGui();
    void handleAdditionalActionRequest(Core::Id actionId);
    void displayCurrent();
    void setDeviceInfoWidgetsEnabled(bool enable);
    IDevice::ConstPtr currentDevice() const;
    int currentIndex() const;
    void clearDetails();
    QString parseTestOutput();
    void fillInValues();
    void updateDeviceFromUi();

    Ui::DeviceSettingsWidget *m_ui;
    DeviceManager * const m_deviceManager;
    DeviceManagerModel * const m_deviceManagerModel;
    NameValidator * const m_nameValidator;
    QList<QPushButton *> m_additionalActionButtons;
    IDeviceWidget *m_configWidget;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // DEVICESETTINGSWIDGET_H
