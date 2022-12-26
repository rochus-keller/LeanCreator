/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
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

#include "deviceusedportsgatherer.h"

#include <utils/portlist.h>
#include <utils/qtcassert.h>
#ifndef QT_NO_SSH
#include <ssh/sshconnection.h>
#include <ssh/sshconnectionmanager.h>
#include <ssh/sshremoteprocess.h>

using namespace QSsh;
#endif
using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class DeviceUsedPortsGathererPrivate
{
 public:
#ifndef QT_NO_SSH
    SshConnection *connection;
    SshRemoteProcess::Ptr process;
#endif
    QList<int> usedPorts;
    QByteArray remoteStdout;
    QByteArray remoteStderr;
    IDevice::ConstPtr device;
};

} // namespace Internal

DeviceUsedPortsGatherer::DeviceUsedPortsGatherer(QObject *parent) :
    QObject(parent), d(new Internal::DeviceUsedPortsGathererPrivate)
{
#ifndef QT_NO_SSH
    d->connection = 0;
#endif
}

DeviceUsedPortsGatherer::~DeviceUsedPortsGatherer()
{
    stop();
    delete d;
}

void DeviceUsedPortsGatherer::start(const IDevice::ConstPtr &device)
{
#ifndef QT_NO_SSH
    QTC_ASSERT(!d->connection, return);
    QTC_ASSERT(device && device->portsGatheringMethod(), return);

    d->device = device;
    d->connection = QSsh::acquireConnection(device->sshParameters());
    connect(d->connection, SIGNAL(error(QSsh::SshError)), SLOT(handleConnectionError()));
    if (d->connection->state() == SshConnection::Connected) {
        handleConnectionEstablished();
        return;
    }
    connect(d->connection, SIGNAL(connected()), SLOT(handleConnectionEstablished()));
    if (d->connection->state() == SshConnection::Unconnected)
        d->connection->connectToHost();
#endif
}

void DeviceUsedPortsGatherer::handleConnectionEstablished()
{
#ifndef QT_NO_SSH
    const QAbstractSocket::NetworkLayerProtocol protocol
            = d->connection->connectionInfo().localAddress.protocol();
    const QByteArray commandLine = d->device->portsGatheringMethod()->commandLine(protocol);
    d->process = d->connection->createRemoteProcess(commandLine);

    connect(d->process.data(), SIGNAL(closed(int)), SLOT(handleProcessClosed(int)));
    connect(d->process.data(), SIGNAL(readyReadStandardOutput()), SLOT(handleRemoteStdOut()));
    connect(d->process.data(), SIGNAL(readyReadStandardError()), SLOT(handleRemoteStdErr()));

    d->process->start();
#endif
}

void DeviceUsedPortsGatherer::stop()
{
#ifndef QT_NO_SSH
    if (!d->connection)
        return;
    d->usedPorts.clear();
    d->remoteStdout.clear();
    d->remoteStderr.clear();
    if (d->process)
        disconnect(d->process.data(), 0, this, 0);
    d->process.clear();
    disconnect(d->connection, 0, this, 0);
    QSsh::releaseConnection(d->connection);
    d->connection = 0;
#endif
}

int DeviceUsedPortsGatherer::getNextFreePort(PortList *freePorts) const
{
    while (freePorts->hasMore()) {
        const int port = freePorts->getNext();
        if (!d->usedPorts.contains(port))
            return port;
    }
    return -1;
}

QList<int> DeviceUsedPortsGatherer::usedPorts() const
{
    return d->usedPorts;
}

void DeviceUsedPortsGatherer::setupUsedPorts()
{
    d->usedPorts.clear();
    const QList<int> usedPorts = d->device->portsGatheringMethod()->usedPorts(d->remoteStdout);
    foreach (const int port, usedPorts) {
        if (d->device->freePorts().contains(port))
            d->usedPorts << port;
    }
    emit portListReady();
}

void DeviceUsedPortsGatherer::handleConnectionError()
{
#ifndef QT_NO_SSH
    if (!d->connection)
        return;
    emit error(tr("Connection error: %1").arg(d->connection->errorString()));
#endif
    stop();
}

void DeviceUsedPortsGatherer::handleProcessClosed(int exitStatus)
{
#ifndef QT_NO_SSH
    if (!d->connection)
        return;
    QString errMsg;
    switch (exitStatus) {
    case SshRemoteProcess::FailedToStart:
        errMsg = tr("Could not start remote process: %1").arg(d->process->errorString());
        break;
    case SshRemoteProcess::CrashExit:
        errMsg = tr("Remote process crashed: %1").arg(d->process->errorString());
        break;
    case SshRemoteProcess::NormalExit:
        if (d->process->exitCode() == 0)
            setupUsedPorts();
        else
            errMsg = tr("Remote process failed; exit code was %1.").arg(d->process->exitCode());
        break;
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO, "Invalid exit status");
    }

    if (!errMsg.isEmpty()) {
        if (!d->remoteStderr.isEmpty()) {
            errMsg += QLatin1Char('\n');
            errMsg += tr("Remote error output was: %1")
                .arg(QString::fromUtf8(d->remoteStderr));
        }
        emit error(errMsg);
    }
#endif
    stop();
}

void DeviceUsedPortsGatherer::handleRemoteStdOut()
{
#ifndef QT_NO_SSH
    if (d->process)
        d->remoteStdout += d->process->readAllStandardOutput();
#endif
}

void DeviceUsedPortsGatherer::handleRemoteStdErr()
{
#ifndef QT_NO_SSH
    if (d->process)
        d->remoteStderr += d->process->readAllStandardError();
#endif
}

} // namespace ProjectExplorer
