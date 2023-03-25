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
    // TODO
}

void LldbEngine2::debugLastCommand()
{
    // TODO
}

void LldbEngine2::setupEngine()
{
    // called when "Start debugging" is pressed.
    // TODO
    notifyEngineSetupOk(); // causes setupInferior
}

void LldbEngine2::setupInferior()
{
    // TODO
    // we come here as reaction to notifyEngineSetupOk
    notifyInferiorSetupOk(); // causes runEngine
}

void LldbEngine2::runEngine()
{
    // TODO
    if (runParameters().useContinueInsteadOfRun)
        notifyInferiorRunOk(); // continue
    else
        // we come here as reaction to notifyInferiorSetupOk
        notifyEngineRunAndInferiorRunOk();
}

void LldbEngine2::shutdownInferior()
{
    // called after interruptInferior in case of Debug/Stop
    // TODO
    notifyInferiorShutdownOk();
}

void LldbEngine2::shutdownEngine()
{
    // called after shutdownInferior in case of Debug/Stop
    // TODO
    notifyEngineShutdownOk();
}

void LldbEngine2::continueInferior()
{
    // happens if Debug/contiue (Continue button)
    // TODO
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
    // TODO
    notifyInferiorStopOk();
}

void LldbEngine2::executeStepOut()
{
    // TODO
    notifyInferiorStopOk();
}

void LldbEngine2::executeNext()
{
    // TODO
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


} // namespace Internal
} // namespace Debugger
