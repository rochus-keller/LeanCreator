/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2023 Rochus Keller (me@rochus-keller.ch) for LeanCreator
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

#ifndef DEBUGGER_LLDBENGINE2
#define DEBUGGER_LLDBENGINE2

#include <debugger/debuggerengine.h>
#include <debugger/disassembleragent.h>
#include <debugger/memoryagent.h>
#include <debugger/watchhandler.h>
#include <debugger/debuggertooltipmanager.h>
#include <debugger/debuggerprotocol.h>

#include <utils/consoleprocess.h>
#include <utils/qtcprocess.h>

#include <QPointer>
#include <QProcess>
#include <QQueue>
#include <QMap>
#include <QStack>
#include <QVariant>


namespace Debugger {
namespace Internal {

class WatchData;
class GdbMi;


class LldbEngine2 : public DebuggerEngine
{
    Q_OBJECT

public:
    explicit LldbEngine2(const DebuggerRunParameters &runParameters);
    ~LldbEngine2();

    DebuggerEngine *cppEngine() override { return this; }

    bool hasCapability(unsigned cap) const override;

    void abortDebugger() override;

    bool canHandleToolTip(const DebuggerToolTipContext &) const override;

    void activateFrame(int index) override;
    void selectThread(ThreadId threadId) override;

    // This should be always the last call in a function.
    bool stateAcceptsBreakpointChanges() const override;
    bool acceptsBreakpoint(Breakpoint bp) const override;
    void insertBreakpoint(Breakpoint bp) override;
    void removeBreakpoint(Breakpoint bp) override;
    void changeBreakpoint(Breakpoint bp) override;

    void assignValueInDebugger(WatchItem *item, const QString &expr, const QVariant &value) override;

    void executeDebuggerCommand(const QString &command, DebuggerLanguages languages) override;

    void loadSymbols(const QString &moduleName) override;
    void loadAllSymbols() override;
    void requestModuleSymbols(const QString &moduleName) override;

    void reloadModules() override;
    void reloadRegisters() override;
    void reloadFullStack() override;
    void reloadDebuggingHelpers() override;

    void fetchDisassembler(Internal::DisassemblerAgent *) override;

    bool isSynchronous() const override { return true; }

    void setRegisterValue(const QByteArray &name, const QString &value) override;

    void fetchMemory(Internal::MemoryAgent *, QObject *, quint64 addr, quint64 length) override;
    void changeMemory(Internal::MemoryAgent *, QObject *, quint64 addr, const QByteArray &data) override;

    void updateAll() override;

    void runCommand(const DebuggerCommand &cmd) override;
    void debugLastCommand() override;

signals:
    void outputReady(const QByteArray &data);

protected:
    void setupEngine() override;
    void setupInferior() override;
    void runEngine() override;
    void shutdownInferior() override;
    void shutdownEngine() override;

    void continueInferior() override;
    void interruptInferior() override;

    void executeStep() override;
    void executeStepOut() override;
    void executeNext() override;
    void executeStepI() override; // by instruction
    void executeNextI() override;

    void executeRunToLine(const ContextData &data) override;
    void executeRunToFunction(const QString &functionName) override;
    void executeJumpToLine(const ContextData &data) override;

    void updateLocals() override;
    void doUpdateLocals(const UpdateParameters &params) override;

protected:
    void timerEvent(QTimerEvent *event);
    void updateProcStat(QByteArrayList &data);
    void updateBreakpoint(QByteArrayList &data);
    void updateStack(QByteArrayList &data);
    void updateVar(QByteArrayList &data);
    QByteArray fetchQByteArray(const QByteArray& desig);
    QString fetchQString(const QByteArray& desig);
    QByteArray fetchCString(const QByteArray& desig);

private slots:
    void handleResponse(const QByteArray& data);
    void handleLldbError(QProcess::ProcessError);
    void handleLldbFinished(int,QProcess::ExitStatus);
    void readLldbStandardOutput();
    void readLldbStandardError();
    void fetchFullBacktrace();
    void stdoutReady();
    void stderrReady();

private:
    DebuggerCommand m_lastDebuggableCommand;

    Utils::QtcProcess m_lldb;
    QString m_lldbCmd;
    QFile* d_out;
    QFile* d_err;
    QByteArray d_curDesig;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_LLDBENGINE2
