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

#include "lldbengine2.h"

#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerdialogs.h>
#include <debugger/debuggerinternalconstants.h>
#include <debugger/debuggermainwindow.h>
#include <debugger/debuggerprotocol.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerstringutils.h>
#include <debugger/debuggertooltipmanager.h>
#include <debugger/debuggeritemmanager.h>

#include <debugger/breakhandler.h>
#include <debugger/disassemblerlines.h>
#include <debugger/moduleshandler.h>
#include <debugger/registerhandler.h>
#include <debugger/stackhandler.h>
#include <debugger/sourceutils.h>
#include <debugger/threadshandler.h>
#include <debugger/watchhandler.h>
#include <debugger/watchutils.h>

#include <core/messagebox.h>
#include <core/idocument.h>
#include <core/icore.h>

#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/qtcprocess.h>

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QToolTip>
#include <QVariant>
#include <QJsonArray>

using namespace Core;
using namespace Utils;

namespace Debugger {
namespace Internal {

DebuggerEngine *createLldbEngine(const DebuggerRunParameters &startParameters)
{
    return new LldbEngine2(startParameters);
}

LldbEngine2::LldbEngine2(const DebuggerRunParameters &startParameters)
    : DebuggerEngine(startParameters)
{
    setObjectName(QLatin1String("LldbEngine2"));



    /* TODO
    connect(action(AutoDerefPointers), &SavedAction::valueChanged,
            this, &LldbEngine::updateLocals);
    connect(action(CreateFullBacktrace), &QAction::triggered,
            this, &LldbEngine::fetchFullBacktrace);
    connect(action(UseDebuggingHelpers), &SavedAction::valueChanged,
            this, &LldbEngine::updateLocals);
    connect(action(UseDynamicType), &SavedAction::valueChanged,
            this, &LldbEngine::updateLocals);
    connect(action(IntelFlavor), &SavedAction::valueChanged,
            this, &LldbEngine::updateAll);
            */
}

LldbEngine2::~LldbEngine2()
{
}

bool LldbEngine2::hasCapability(unsigned cap) const
{
    if (cap &
        ( 0
        //| ReverseSteppingCapability
        //| AutoDerefPointersCapability
        //| DisassemblerCapability
        //| RegisterCapability
        //| ShowMemoryCapability
        //| JumpToLineCapability
        //| ReloadModuleCapability
        //| ReloadModuleSymbolsCapability
        //| BreakOnThrowAndCatchCapability
        //| BreakConditionCapability
        //| TracePointCapability
        //| ReturnFromFunctionCapability
        //| CreateFullBacktraceCapability
        //| WatchpointByAddressCapability
        //| WatchpointByExpressionCapability
        //| AddWatcherCapability
        //| WatchWidgetsCapability
        //| ShowModuleSymbolsCapability
        //| ShowModuleSectionsCapability
        //| CatchCapability
        //| OperateByInstructionCapability
        //| RunToLineCapability
        //| WatchComplexExpressionsCapability
        //| MemoryAddressCapability
          ))
        return true;

    if (runParameters().startMode != StartInternal ) // == AttachCore)
        return false;

    return false;
}

void LldbEngine2::abortDebugger()
{
    // TODO
}

bool LldbEngine2::canHandleToolTip(const DebuggerToolTipContext &) const
{
    return false; // TODO
}

void LldbEngine2::activateFrame(int index)
{
    // TODO
}

void LldbEngine2::selectThread(ThreadId threadId)
{
    // TODO
}

bool LldbEngine2::stateAcceptsBreakpointChanges() const
{
    // TODO
}

bool LldbEngine2::acceptsBreakpoint(Breakpoint bp) const
{
    // TODO
    return true;
}

void LldbEngine2::insertBreakpoint(Breakpoint bp)
{
    // TODO
}

void LldbEngine2::removeBreakpoint(Breakpoint bp)
{
    // TODOs
}

void LldbEngine2::changeBreakpoint(Breakpoint bp)
{
    // TODO
}

void LldbEngine2::assignValueInDebugger(WatchItem *item, const QString &expr, const QVariant &value)
{
    // TODO
}

void LldbEngine2::executeDebuggerCommand(const QString &command, DebuggerLanguages languages)
{
    // TODO
}

void LldbEngine2::loadSymbols(const QString &moduleName)
{
    // TODO
}

void LldbEngine2::loadAllSymbols()
{
    // TODO
}

void LldbEngine2::requestModuleSymbols(const QString &moduleName)
{
    // TODO
}

void LldbEngine2::reloadModules()
{
    // TODO
}

void LldbEngine2::reloadRegisters()
{
    // TODO
}

void LldbEngine2::reloadFullStack()
{
    // TODO
}

void LldbEngine2::reloadDebuggingHelpers()
{
    // TODO
}

void LldbEngine2::fetchDisassembler(DisassemblerAgent *)
{
    // TODO
}

void LldbEngine2::setRegisterValue(const QByteArray &name, const QString &value)
{
    // TODO
}

void LldbEngine2::fetchMemory(MemoryAgent *, QObject *, quint64 addr, quint64 length)
{
    // TODO
}

void LldbEngine2::changeMemory(MemoryAgent *, QObject *, quint64 addr, const QByteArray &data)
{
    // TODO
}

void LldbEngine2::updateAll()
{
    // TODO
}

void LldbEngine2::runCommand(const DebuggerCommand &cmd)
{
    QTC_ASSERT(m_lldb.state() == QProcess::Running, notifyEngineIll());
    // TODO
}

void LldbEngine2::debugLastCommand()
{
    // TODO
}

void LldbEngine2::setupEngine()
{
    // called when "Start debugging" is pressed.

    if (runParameters().useTerminal) {
        showMessage("debugging terminal app not yet supported");
        notifyEngineSetupFailed();
    }else if (runParameters().remoteSetupNeeded)
    {
        showMessage("debugging remote apps not yet supported");
        notifyEngineSetupFailed();
    }else
    {
        m_lldbCmd = runParameters().debuggerCommand;

        connect(&m_lldb, SIGNAL(error(QProcess::ProcessError)),this, SLOT(handleLldbError(QProcess::ProcessError)));
        connect(&m_lldb, SIGNAL(finished(int,QProcess::ExitStatus)),this, SLOT(handleLldbFinished(int,QProcess::ExitStatus)));
        connect(&m_lldb, SIGNAL(readyReadStandardOutput()), this, SLOT(readLldbStandardOutput()));
        connect(&m_lldb, SIGNAL(readyReadStandardError()), this, SLOT(readLldbStandardError()));
        connect( this, SIGNAL(outputReady(QByteArray)), this, SLOT(handleResponse(QByteArray)));

        showMessage(_("STARTING LLDB: ") + m_lldbCmd);
        m_lldb.setEnvironment(runParameters().environment);
        if (!runParameters().workingDirectory.isEmpty())
            m_lldb.setWorkingDirectory(runParameters().workingDirectory);

        m_lldb.setCommand(m_lldbCmd, QString());
        m_lldb.start();

        if (!m_lldb.waitForStarted()) {
            const QString msg = tr("Unable to start LLDB \"%1\": %2").arg(m_lldbCmd, m_lldb.errorString());
            notifyEngineSetupFailed();
            showMessage(_("ADAPTER START FAILED"));
            if (!msg.isEmpty())
                ICore::showWarningWithOptions(tr("Adapter start failed."), msg);
            return;
        }
        m_lldb.waitForReadyRead(1000);
        notifyEngineSetupOk(); // causes setupInferior
    }
}

void LldbEngine2::setupInferior()
{
    // we come here as reaction to notifyEngineSetupOk

    Environment sysEnv = Environment::systemEnvironment();
    Environment runEnv = runParameters().environment;
    foreach (const EnvironmentItem &item, sysEnv.diff(runEnv)) {
        QByteArray cmd;
        if (item.unset)
            cmd = "settings remove target.env-vars " + item.name.toUtf8();
        else
            cmd = "settings set target.env-vars " + item.name.toUtf8() + '=' + item.value.toUtf8();
        m_lldb.write(cmd + "\n");
    }

    const DebuggerRunParameters &rp = runParameters();

    QString executable;
    QtcProcess::Arguments args;
    QtcProcess::prepareCommand(QFileInfo(rp.executable).absoluteFilePath(),
                               rp.processArgs, &executable, &args);

    m_lldb.write( "file " + executable.toUtf8() + "\n" );

    // TODO: set breakpoints

    notifyInferiorSetupOk(); // causes runEngine
}

void LldbEngine2::runEngine()
{
    // TODO

    const DebuggerRunParameters& rp = runParameters();

    QByteArray cmd;
    if( rp.breakOnMain )
        cmd = "process launch --stop-at-entry";
    else
        cmd = "process launch";
    m_lldb.write( cmd + "\n" );

    // we come here as reaction to notifyInferiorSetupOk
    notifyEngineRunAndInferiorRunOk();

    if( rp.breakOnMain )
        notifyInferiorStopOk();
}

void LldbEngine2::shutdownInferior()
{
    // called after interruptInferior in case of Debug/Stop
    // TODO
    m_lldb.write( "process kill\n" );
    notifyInferiorShutdownOk();
}

void LldbEngine2::shutdownEngine()
{
    // called after shutdownInferior in case of Debug/Stop
    // TODO
    if( m_lldb.state() == QProcess::Running )
        m_lldb.write( "quit\n" );
        //m_lldb.terminate();
    notifyEngineShutdownOk();
}

void LldbEngine2::continueInferior()
{
    // happens if Debug/contiue (Continue button)
    m_lldb.write( "thread continue\n" );
    notifyInferiorRunOk();
}

void LldbEngine2::interruptInferior()
{
    // happens if Debug/interrupt (Pause button) or Debug/stop
    // TODO
    notifyInferiorStopOk();
    // notifyInferiorSpontaneousStop();
}

void LldbEngine2::executeStep()
{
    m_lldb.write( "thread step-in\n" );
    notifyInferiorStopOk();
}

void LldbEngine2::executeStepOut()
{
    m_lldb.write( "thread step-out\n" );
    notifyInferiorStopOk();
}

void LldbEngine2::executeNext()
{
    m_lldb.write( "thread step-over\n" );
    notifyInferiorStopOk();
}

void LldbEngine2::executeStepI()
{
    // TODO
    notifyInferiorStopOk();
}

void LldbEngine2::executeNextI()
{
    // TODO
    notifyInferiorStopOk();
}

void LldbEngine2::executeRunToLine(const ContextData &data)
{
    // TODO
    notifyInferiorStopOk();
}

void LldbEngine2::executeRunToFunction(const QString &functionName)
{
    // TODO
    notifyInferiorStopOk();
}

void LldbEngine2::executeJumpToLine(const ContextData &data)
{
    // TODO
    notifyInferiorStopOk();
}

void LldbEngine2::doUpdateLocals(const UpdateParameters &params)
{
    // TODO
}

void LldbEngine2::notifyEngineRemoteSetupFinished(const RemoteSetupResult &result)
{
    // TODO
}

void LldbEngine2::handleResponse(const QByteArray &data)
{

}

static QString errorMessage(QProcess::ProcessError error, const QString& cmd)
{
    switch (error) {
        case QProcess::FailedToStart:
            return LldbEngine2::tr("The LLDB process failed to start. Either the "
                "invoked program \"%1\" is missing, or you may have insufficient "
                "permissions to invoke the program.")
                .arg(cmd);
        case QProcess::Crashed:
            return LldbEngine2::tr("The LLDB process crashed some time after starting "
                "successfully.");
        case QProcess::Timedout:
            return LldbEngine2::tr("The last waitFor...() function timed out. "
                "The state of QProcess is unchanged, and you can try calling "
                "waitFor...() again.");
        case QProcess::WriteError:
            return LldbEngine2::tr("An error occurred when attempting to write "
                "to the LLDB process. For example, the process may not be running, "
                "or it may have closed its input channel.");
        case QProcess::ReadError:
            return LldbEngine2::tr("An error occurred when attempting to read from "
                "the Lldb process. For example, the process may not be running.");
        default:
            return LldbEngine2::tr("An unknown error in the LLDB process occurred.") + QLatin1Char(' ');
    }
}

void LldbEngine2::handleLldbError(QProcess::ProcessError error)
{
    showMessage(_("LLDB PROCESS ERROR: %1").arg(error));
    switch (error) {
    case QProcess::Crashed:
        break; // will get a processExited() as well
    // impossible case QProcess::FailedToStart:
    case QProcess::ReadError:
    case QProcess::WriteError:
    case QProcess::Timedout:
    default:
        //setState(EngineShutdownRequested, true);
        m_lldb.kill();
        AsynchronousMessageBox::critical(tr("LLDB I/O Error"), errorMessage(error, m_lldbCmd));
        break;
    }
}

void LldbEngine2::handleLldbFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    notifyDebuggerProcessFinished(exitCode, exitStatus, QLatin1String("LLDB"));
}

void LldbEngine2::readLldbStandardOutput()
{
    QByteArray out = m_lldb.readAllStandardOutput();
    out.replace("\r\n", "\n");
    qDebug() << "*** LLDB stdout:" << out;
    showMessage(QString::fromUtf8(out), LogOutput);
    emit outputReady(out);
}

void LldbEngine2::readLldbStandardError()
{
    QByteArray err = m_lldb.readAllStandardError();
    qDebug() << "*** LLDB stderr:" << err;
    showMessage("Lldb stderr: " + QString::fromUtf8(err), LogError);
}


} // namespace Internal
} // namespace Debugger
