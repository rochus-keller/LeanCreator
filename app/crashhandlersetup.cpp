/**************************************************************************
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

#include "crashhandlersetup.h"

#include <QtGlobal>

#if !defined(QT_NO_DEBUG) && defined(Q_OS_LINUX)
#define BUILD_CRASH_HANDLER
#endif

#ifdef BUILD_CRASH_HANDLER

#include <QApplication>
#include <QString>

#include <stdlib.h>

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/prctl.h>

// Enable compilation with older header that doesn't contain this constant
// for running on newer libraries that do support it
#ifndef PR_SET_PTRACER
#define PR_SET_PTRACER 0x59616d61
#endif

#ifdef Q_WS_X11
#include <qx11info_x11.h>
#include <X11/Xlib.h>
#endif

static const char *crashHandlerPathC;
static void *signalHandlerStack;

extern "C" void signalHandler(int signal)
{
#ifdef Q_WS_X11
    // Kill window since it's frozen anyway.
    if (QX11Info::display())
        close(ConnectionNumber(QX11Info::display()));
#endif
    pid_t pid = fork();
    switch (pid) {
    case -1: // error
        break;
    case 0: // child
        execl(crashHandlerPathC, crashHandlerPathC, strsignal(signal), (char *) 0);
        _exit(EXIT_FAILURE);
    default: // parent
        prctl(PR_SET_PTRACER, pid, 0, 0, 0);
        waitpid(pid, 0, 0);
        _exit(EXIT_FAILURE);
        break;
    }
}
#endif // BUILD_CRASH_HANDLER

CrashHandlerSetup::CrashHandlerSetup()
{
#ifdef BUILD_CRASH_HANDLER
    if (qgetenv("QTC_USE_CRASH_HANDLER").isEmpty())
        return;

    const QString crashHandlerPath = qApp->applicationDirPath()
        + QLatin1String("/qtcreator_crash_handler");
    crashHandlerPathC = qstrdup(qPrintable(crashHandlerPath));

    // Setup an alternative stack for the signal handler. This way we are able to handle SIGSEGV
    // even if the normal process stack is exhausted.
    stack_t ss;
    ss.ss_sp = signalHandlerStack = malloc(SIGSTKSZ); // Usual requirements for alternative signal stack.
    if (ss.ss_sp == 0) {
        qWarning("Warning: Could not allocate space for alternative signal stack (%s).", Q_FUNC_INFO);
        return;
    }
    ss.ss_size = SIGSTKSZ;
    ss.ss_flags = 0;
    if (sigaltstack(&ss, 0) == -1) {
        qWarning("Warning: Failed to set alternative signal stack (%s).", Q_FUNC_INFO);
        return;
    }

    // Install signal handler for calling the crash handler.
    struct sigaction sa;
    if (sigemptyset(&sa.sa_mask) == -1) {
        qWarning("Warning: Failed to empty signal set (%s).", Q_FUNC_INFO);
        return;
    }
    sa.sa_handler = &signalHandler;
    // SA_RESETHAND - Restore signal action to default after signal handler has been called.
    // SA_NODEFER - Don't block the signal after it was triggered (otherwise blocked signals get
    // inherited via fork() and execve()). Without this the signal will not be delivered to the
    // restarted LeanCreator.
    // SA_ONSTACK - Use alternative stack.
    sa.sa_flags = SA_RESETHAND | SA_NODEFER | SA_ONSTACK;
    // See "man 7 signal" for an overview of signals.
    // Do not add SIGPIPE here, QProcess and QTcpSocket use it.
    const int signalsToHandle[] = { SIGILL, SIGABRT, SIGFPE, SIGSEGV, SIGBUS, 0 };
    for (int i = 0; signalsToHandle[i]; ++i) {
        if (sigaction(signalsToHandle[i], &sa, 0) == -1 ) {
            qWarning("Warning: Failed to install signal handler for signal \"%s\" (%s).",
                strsignal(signalsToHandle[i]), Q_FUNC_INFO);
        }
    }
#endif // BUILD_CRASH_HANDLER
}

CrashHandlerSetup::~CrashHandlerSetup()
{
#ifdef BUILD_CRASH_HANDLER
    delete[] crashHandlerPathC;
    free(signalHandlerStack);
#endif
}
