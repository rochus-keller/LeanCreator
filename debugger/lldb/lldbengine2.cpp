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

#include "lldbengine2.h"
#include "lldbengine.h"

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
#include <debugger/procinterrupt.h>

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
#include <QTemporaryFile>

using namespace Core;
using namespace Utils;

namespace Debugger {
namespace Internal {

#if 1
DebuggerEngine *createLldbEngine(const DebuggerRunParameters &startParameters)
{
    QString v = startParameters.debuggerVersion;
    const int nl = v.indexOf('\n');
    if( nl != -1 )
        v = v.left(nl);
    const QStringList parts = v.split('.');
    const int maj = parts.first().toInt();
    if( maj == 0 || ( maj < 100 && maj >= 4) || maj >= 400 ) // TODO: verify
        return new LldbEngine2(startParameters);
    else
        return new LldbEngine(startParameters);
}
#endif

LldbEngine2::LldbEngine2(const DebuggerRunParameters &startParameters)
    : DebuggerEngine(startParameters),d_out(0), d_err(0)
{
    setObjectName(QLatin1String("LldbEngine2"));



    connect(action(AutoDerefPointers), &SavedAction::valueChanged,
            this, &LldbEngine2::updateLocals);
    connect(action(UseDebuggingHelpers), &SavedAction::valueChanged,
            this, &LldbEngine2::updateLocals);
    connect(action(UseDynamicType), &SavedAction::valueChanged,
            this, &LldbEngine2::updateLocals);
    connect(action(IntelFlavor), &SavedAction::valueChanged,
            this, &LldbEngine2::updateAll);
    connect(action(CreateFullBacktrace), &QAction::triggered,
            this, &LldbEngine2::fetchFullBacktrace);

    startTimer(200);
}

LldbEngine2::~LldbEngine2()
{
    m_lldb.disconnect();
}

bool LldbEngine2::hasCapability(unsigned cap) const
{
    if (cap &
        ( 0
//        | ReverseSteppingCapability
//        | AutoDerefPointersCapability
//        | DisassemblerCapability
//        | RegisterCapability
//        | ShowMemoryCapability
//        | JumpToLineCapability
//        | ReloadModuleCapability
//        | ReloadModuleSymbolsCapability
//        | BreakOnThrowAndCatchCapability
//        | BreakConditionCapability
//        | TracePointCapability
//        | ReturnFromFunctionCapability
//        | CreateFullBacktraceCapability
//        | WatchpointByAddressCapability
//        | WatchpointByExpressionCapability
//        | AddWatcherCapability
//        | WatchWidgetsCapability
//        | ShowModuleSymbolsCapability
//        | ShowModuleSectionsCapability
//        | CatchCapability
//        | OperateByInstructionCapability
//        | RunToLineCapability
//        | WatchComplexExpressionsCapability
//        | MemoryAddressCapability
          ))
        return true;

    if (runParameters().startMode != StartInternal ) // == AttachCore)
        return false;

    return false;
}

void LldbEngine2::abortDebugger()
{
    // TODO
    qWarning() << "LldbEngine2::abortDebugger not yet implemented";
}

bool LldbEngine2::canHandleToolTip(const DebuggerToolTipContext & ctx) const
{
    qWarning() << "LldbEngine2::canHandleToolTip not yet implemented";
    return false; // TODO
}

void LldbEngine2::activateFrame(int frameIndex)
{
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    m_lldb.write("frame select " + QByteArray::number(frameIndex) + "\n");
    StackHandler *handler = stackHandler();
    QTC_ASSERT(frameIndex < handler->stackSize(), return);
    handler->setCurrentIndex(frameIndex);
    gotoLocation(handler->currentFrame());
    updateLocals();
}

void LldbEngine2::selectThread(ThreadId threadId)
{
    qWarning() << "LldbEngine2::selectThread not yet implemented";
    // TODO
}

bool LldbEngine2::stateAcceptsBreakpointChanges() const
{
    switch (state()) {
    case InferiorSetupRequested:
    case InferiorSetupOk:
    case EngineRunRequested:
    case InferiorRunRequested:
    case InferiorRunOk:
    case InferiorStopRequested:
    case InferiorStopOk:
        return true;
    default:
        return false;
    }
}

bool LldbEngine2::acceptsBreakpoint(Breakpoint bp) const
{
    if (runParameters().startMode == AttachCore)
        return false;
    return bp.parameters().isCppBreakpoint();
}

void LldbEngine2::insertBreakpoint(Breakpoint bp)
{
    const BreakpointParameters & p = bp.parameters();
    if( p.type != BreakpointByFileAndLine )
    {
        showMessage("breakpoint type not yet supported");
        bp.notifyBreakpointInsertFailed();
        return;
    }
    bp.notifyBreakpointInsertProceeding();
    QByteArray cmd = "breakpoint set --file ";
    cmd += p.fileName.toUtf8();
    cmd += " --line ";
    cmd += QByteArray::number(p.lineNumber);
    cmd += " --breakpoint-name BP";
    cmd += bp.id().toByteArray();
    m_lldb.write(cmd + "\n");
    bp.notifyBreakpointInsertOk();
}

void LldbEngine2::removeBreakpoint(Breakpoint bp)
{
    bp.notifyBreakpointRemoveProceeding();
    QByteArray cmd = "breakpoint delete BP";
    cmd += bp.id().toByteArray();
    m_lldb.write(cmd + "\n");
    bp.notifyBreakpointRemoveOk();
}

void LldbEngine2::changeBreakpoint(Breakpoint bp)
{
    // TODO
    qWarning() << "LldbEngine2::changeBreakpoint not yet implemented";
}

void LldbEngine2::assignValueInDebugger(WatchItem *item, const QString &expr, const QVariant &value)
{
    // TODO
    qWarning() << "LldbEngine2::assignValueInDebugger not yet implemented";
}

void LldbEngine2::executeDebuggerCommand(const QString &command, DebuggerLanguages languages)
{
    QTC_ASSERT(m_lldb.state() == QProcess::Running, return);
    m_lldb.write(command.toUtf8() + "\n");
}

void LldbEngine2::loadSymbols(const QString &moduleName)
{
    // TODO
    qWarning() << "LldbEngine2::loadSymbols not yet implemented";
}

void LldbEngine2::loadAllSymbols()
{
    // TODO
    qWarning() << "LldbEngine2::loadAllSymbols not yet implemented";
}

void LldbEngine2::requestModuleSymbols(const QString &moduleName)
{
    // TODO
    qWarning() << "LldbEngine2::requestModuleSymbols not yet implemented";
}

void LldbEngine2::reloadModules()
{
    // TODO
    qWarning() << "LldbEngine2::reloadModules not yet implemented";
}

void LldbEngine2::reloadRegisters()
{
    // TODO
    qWarning() << "LldbEngine2::reloadRegisters not yet implemented";
}

void LldbEngine2::reloadFullStack()
{
    // TODO
    qWarning() << "LldbEngine2::reloadFullStack not yet implemented";
}

void LldbEngine2::reloadDebuggingHelpers()
{
    // TODO
    qWarning() << "LldbEngine2::reloadDebuggingHelpers not yet implemented";
}

void LldbEngine2::fetchDisassembler(DisassemblerAgent *)
{
    // TODO
    qWarning() << "LldbEngine2::fetchDisassembler not yet implemented";
}

void LldbEngine2::setRegisterValue(const QByteArray &name, const QString &value)
{
    // TODO
    qWarning() << "LldbEngine2::setRegisterValue not yet implemented";
}

void LldbEngine2::fetchMemory(MemoryAgent *, QObject *, quint64 addr, quint64 length)
{
    // TODO
    qWarning() << "LldbEngine2::fetchMemory not yet implemented";
}

void LldbEngine2::changeMemory(MemoryAgent *, QObject *, quint64 addr, const QByteArray &data)
{
    // TODO
    qWarning() << "LldbEngine2::changeMemory not yet implemented";
}

void LldbEngine2::updateAll()
{
    m_lldb.write("thread backtrace\n");
    watchHandler()->resetValueCache();
    updateLocals();
}

void LldbEngine2::runCommand(const DebuggerCommand &cmd)
{
    QTC_ASSERT(m_lldb.state() == QProcess::Running, notifyEngineIll());
    // TODO
    qWarning() << "LldbEngine2::runCommand not yet implemented";
}

void LldbEngine2::debugLastCommand()
{
    // TODO
    qWarning() << "LldbEngine2::debugLastCommand not yet implemented";
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

    m_lldb.write("settings set frame-format \""
                    "frame ${frame.index}"
                    "{ #mod ${module.file.fullpath}{:${function.name}}}##"
                    "{ #src ${line.file.fullpath}:${line.number}}##"
                 "\\n\"\n" );


    const QByteArray pyPath =
#if 1
            DebuggerItemManager::externalsPath() + "/lldbformatter.py";
#else
            "/home/me/Projects/LeanCreator/debugger/python/lldbformatter.py";
#endif
    m_lldb.write("command script import " + pyPath + "\n" );

    const DebuggerRunParameters &rp = runParameters();

    QString executable;
    QtcProcess::Arguments args;
    QtcProcess::prepareCommand(QFileInfo(rp.executable).absoluteFilePath(),
                               rp.processArgs, &executable, &args);


    m_lldb.write( "file " + executable.toUtf8() + "\n" );
    // stdout:
    // Current executable set to
}

void LldbEngine2::runEngine()
{
    attemptBreakpointSynchronization();

    QByteArray cmd = "process launch ";

    d_out = new QTemporaryFile(this);
    d_out->open(QIODevice::ReadOnly);
    cmd += "--stdout " + d_out->fileName().toUtf8();

    d_err = new QTemporaryFile(this);
    d_err->open(QIODevice::ReadOnly);
    cmd += " --stderr " + d_err->fileName().toUtf8();

    if( runParameters().breakOnMain )
        cmd = " --stop-at-entry";
    m_lldb.write( cmd + "\n" );

    notifyInferiorRunRequested();

    // we come here as reaction to notifyInferiorSetupOk
}

void LldbEngine2::shutdownInferior()
{
    // called after interruptInferior in case of Debug/Stop
    m_lldb.write( "process kill\n" );
}

void LldbEngine2::shutdownEngine()
{
    // called after shutdownInferior in case of Debug/Stop
    if( m_lldb.state() == QProcess::Running )
        m_lldb.write( "quit\n" );
        //m_lldb.terminate();
    notifyEngineShutdownOk();
    if( d_out )
    {
        d_out->deleteLater();
        d_out = 0;
    }
    if( d_err )
    {
        d_err->deleteLater();
        d_err = 0;
    }
}

void LldbEngine2::continueInferior()
{
    // happens if Debug/contiue (Continue button)
    notifyInferiorRunRequested();
    m_lldb.write( "thread continue\n" );
    // stdout
    // "Resuming thread 0x2b56 in process 11094\n
    //  Process 11094 resuming\n"
}

void LldbEngine2::interruptInferior()
{
    // happens if Debug/interrupt (Pause button) or Debug/stop
    QString error;
    interruptProcess(m_lldb.processId(), LldbEngineType, &error);

    // stdout:
    // "Process 11094 stopped\n
    //  * thread #1, name = 'hello', stop reason = signal SIGSTOP\n
    //      frame #0: 0x000000000040243a hello`main(argc=1, argv=0x00007fffffffe8f8) at hello.cpp:55:5\n
    //     52  \t\tl << \"alpha\" << \"beta\" << \"gamma\";\n
    //     53  \t\tqDebug() << hello << QDateTime::currentDateTime().toString(Qt::ISODate) << str << l; // << v;\n
    //     54  \t    std::cout << str.toUtf8().constData() << \" Hello World \" << hello.constData() << std::endl;\n
    //  -> 55  \t    while(1)\n    \t    ^\n
    //     56  \t    {\n
    //     57  \t\n
    //     58  \t    }\n"
}

void LldbEngine2::executeStep()
{
    notifyInferiorRunRequested();
    m_lldb.write( "thread step-in\n" );
    notifyInferiorRunOk();
}

void LldbEngine2::executeStepOut()
{
    notifyInferiorRunRequested();
    m_lldb.write( "thread step-out\n" );
    notifyInferiorRunOk();
}

void LldbEngine2::executeNext()
{
    notifyInferiorRunRequested();
    m_lldb.write( "thread step-over\n" );
    notifyInferiorRunOk();
}

void LldbEngine2::executeStepI()
{
    // TODO
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    qWarning() << "LldbEngine2::executeStepI not yet implemented";
}

void LldbEngine2::executeNextI()
{
    // TODO
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    qWarning() << "LldbEngine2::executeNextI not yet implemented";
}

void LldbEngine2::executeRunToLine(const ContextData &data)
{
    // TODO
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    qWarning() << "LldbEngine2::executeRunToLine not yet implemented";
}

void LldbEngine2::executeRunToFunction(const QString &functionName)
{
    // TODO
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    qWarning() << "LldbEngine2::executeRunToFunction not yet implemented";
}

void LldbEngine2::executeJumpToLine(const ContextData &data)
{
    // TODO
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    qWarning() << "LldbEngine2::executeJumpToLine not yet implemented";
}

void LldbEngine2::updateLocals()
{
    //watchHandler()->notifyUpdateStarted();
    doUpdateLocals(UpdateParameters());
}

void LldbEngine2::doUpdateLocals(const UpdateParameters &params)
{
    if( params.partialVariable.isEmpty() )
    {
        watchHandler()->cleanup();

        /* Just a top-level list of variables without any members:
            (lldb) frame variable -T -D0 -R
            (int) argc = 1
            (char **) argv = 0x00007fffffffe2f8{...}
            (QByteArray) hello ={...}
            (const char *) tmp = 0x00000000004f7a25{...}
            (QString) str ={...}
            (const ushort *) tmp2 = 0x0000000000402079{...}
            (QByteArrayList) l ={...}
           Add -L for address
           add -g for global and static vars
        */
        m_lldb.write("frame variable -T -D0\n");

    }else
    {
        QByteArray desig = params.partialVariable;
        if( desig.startsWith("local.") )
            desig = desig.mid(6);
        desig.replace(".>", "->");
        desig.replace(".[", "[");
        //m_lldb.write("frame variable " + desig + " -F -T -P1\n");
        m_lldb.write("frame variable " + desig + " -T -D1 -P1\n");
    }
}

void LldbEngine2::timerEvent(QTimerEvent *event)
{
    stdoutReady();
    stderrReady();
}

void LldbEngine2::updateProcStat(QByteArrayList &data)
{
    const QByteArrayList parts = data.first().split(' ');
    if( parts.size() > 2 )
    {
        if( parts[2].startsWith("launched") )
        {
            if( runParameters().breakOnMain )
            {
                notifyEngineRunAndInferiorStopOk();
                updateAll();
            }else
                notifyEngineRunAndInferiorRunOk();
        }else if( parts[2] == "stopped" )
        {
            notifyInferiorStopOk();
            updateAll();
        }else if( parts[2] == "exited" )
            notifyInferiorShutdownOk();
        else if( parts[2] == "resuming" )
            notifyInferiorRunOk();
    }
    data.pop_front();
}

void LldbEngine2::updateBreakpoint(QByteArrayList &data)
{
    const QByteArray line = data.first();
    data.pop_front();
}

void LldbEngine2::updateStack(QByteArrayList &data)
{
    StackHandler *handler = stackHandler();
    StackFrames frames;
    int i = 0;
    for( i = 1; i < data.size(); i++ )
    {
        int pos = data[i].indexOf(" frame ");
        if( pos == -1 )
            break;

        StackFrame frame;

        //"frame ${frame.index}"
        //"{ #mod ${module.file.fullpath}{:${function.name}}}##"
        //"{ #src ${line.file.fullpath}:${line.number}}##"

        const int mod = data[i].indexOf("#mod");
        const int src = data[i].indexOf("#src");

        int to = data[i].size();
        if( mod != -1 )
            to = mod;
        else if( src != -1 )
            to = src;

        frame.level = data[i].mid(pos+7,to - pos - 7).trimmed();

        if( src != -1 )
        {
            pos = data[i].indexOf("##", src+4);
            frame.file = data[i].mid(src+4, pos - src -4).trimmed();
            pos = frame.file.indexOf(':');
            if( pos != -1 )
            {
                frame.line = frame.file.mid(pos+1).trimmed().toInt();
                frame.file = frame.file.left(pos);
            }
            frame.usable = QFileInfo(frame.file).isReadable();
        }
        if( mod != -1 )
        {
            pos = data[i].indexOf("##", mod+4);
            frame.module = data[i].mid(mod+4, pos - mod -4).trimmed();
            pos = frame.module.indexOf(':');
            if( pos != -1 )
            {
                frame.function = frame.module.mid(pos+1).trimmed();
                frame.module = frame.module.left(pos);
            }
        }
        frames.append(frame);
    }
    data = data.mid(i);
    handler->setFrames(frames, false);

    const int pos = stackHandler()->firstUsableIndex();
    handler->setCurrentIndex(pos);
    if (pos >= 0 && pos < handler->stackSize())
        gotoLocation(handler->frameAt(pos));
}

void LldbEngine2::updateVar(QByteArrayList &data)
{
    const QByteArray line = data.takeFirst().trimmed();

    /* Here we expect this:
        (int) argc = 1
        (char **) argv = 0x00007fffffffe2f8{...}
        (QByteArray) hello ={...}
        (const char *) tmp = 0x00000000004f7a25{...}
        (QString) str ={...}
        (const ushort *) tmp2 = 0x0000000000402079{...}
        (QByteArrayList) l ={...}
    or this:
        (QByteArray *) *phello = 0x00000000004f7a25 {
    */
    const int rpar = line.indexOf(')');
    if( rpar == -1 )
        return; // This is actually a protocol error

    WatchData v;
    v.setType(line.mid(1,rpar-1).trimmed(),false);

    const int eq = line.indexOf('=',rpar);
    if( eq == -1 )
        return; // This is again a protocol error

    QByteArray name = line.mid(rpar+1, eq - rpar -1).trimmed();
    if( !name.startsWith("[") )
        name.replace("[", ".[");

    if( line.endsWith('{'))
    {
        d_curDesig = name;
        return;
    }


    v.iname = "local.";
    if( !d_curDesig.isEmpty() )
        v.iname += d_curDesig + ".";
    v.iname += name;
    v.name = name;

    //v.iname.replace('-','.'); // convert a.b->c to a.b.>c
    //v.name = v.iname.split('.').last();
    //if(v.name.startsWith('>') )
    //    v.name = v.name.mid(1);

    v.editvalue = line.mid(eq+1).trimmed();
    v.value = v.editvalue;

    bool isByteArray = false;
#if 0
    // NOTE: this perfectly works on Linux, but not on Mac.
    // on Mac we have issues with missed or delayed output from m_lldb
    // and waitForReadyRead timeouts; adding QApp::processEvents helps
    // for the latter but it loses sync with the engine
    if( v.type == "QByteArray" )
    {
        v.editvalue = fetchQByteArray(name);
        v.value = QString("\"%1\"").arg(QString::fromLatin1(v.editvalue));
        //v.editformat = DisplayLatin1String;
        //v.editencoding = Unencoded8Bit;
    }else if( v.type == "QString" )
    {
        const QString str = fetchQString(name);
        v.value = QString("\"%1\"").arg(str);
        v.editvalue = str.toUtf8();
    }else if( v.type == "char *")
    {
        v.editvalue = fetchCString(name);
        v.value = QString::fromLatin1(v.editvalue);
    }
    else
#else
    if( v.editvalue.contains('$') ) // a hex byte string quoted by $$
    {
        const int quote = v.editvalue.indexOf('$');
        if( quote != -1 )
            v.editvalue = v.editvalue.mid(quote);
        const int brace = v.editvalue.indexOf('{');
        if( brace != -1 )
            v.editvalue = v.editvalue.left(brace).trimmed();
        v.editvalue = v.editvalue.mid(1,v.editvalue.size()-2); // remove quotes
        v.editvalue = QByteArray::fromHex(v.editvalue);
        v.value = QString("\"%1\"").arg(QString::fromLatin1(v.editvalue));
        isByteArray = true;
    }
    else if( v.editvalue.contains('"') ) // a utf-8 string quoted by ""
    {
        const int quote = v.editvalue.indexOf('"');
        if( quote != -1 )
            v.editvalue = v.editvalue.mid(quote);
        const int brace = v.editvalue.indexOf('{');
        if( brace != -1 )
            v.editvalue = v.editvalue.left(brace).trimmed();
        v.value = QString::fromUtf8(v.editvalue);
        v.editvalue = v.editvalue.mid(1,v.editvalue.size()-2); // remove quotes
    }else
#endif
    if( v.editvalue.endsWith("{...}") )
    {
        v.wantsChildren = !v.editvalue.contains("size=0");
        v.editvalue.chop(5);
        v.value.chop(5);
    }

    WatchHandler* h = watchHandler();
    h->notifyUpdateStarted(QByteArrayList() << v.iname);

    WatchItem* wi = new WatchItem(v);
    h->insertItem(wi);

    if( isByteArray )
    {
        WatchData vv;
        vv.type = "char";
        for( int i = 0; i < v.editvalue.size(); i++ )
        {
            vv.name = QString("[%1]").arg(i);
            vv.iname = v.iname + vv.name.toUtf8();
            vv.value = QByteArray::number((quint8)v.editvalue[i]);
            const QChar ch = QChar::fromLatin1(v.editvalue[i]);
            if( ch.isPrint() )
                vv.value += QString(" '%1'").arg(ch);
            wi->appendChild( new WatchItem(vv) );
        }
    }

    h->notifyUpdateFinished();

    // DebuggerToolTipManager::updateEngine(this);

}

QByteArray LldbEngine2::fetchQByteArray(const QByteArray &desig)
{
    disconnect(&m_lldb, SIGNAL(readyReadStandardOutput()), this, SLOT(readLldbStandardOutput()));
    m_lldb.write("frame variable " + desig + " -F -T -R -P1\n");
    m_lldb.write("version\n");
    QByteArray data;
    while( true )
    {
        if( !m_lldb.waitForReadyRead() )
            break;
        const QByteArray out = m_lldb.readAllStandardOutput();
        data += out;
        if( out.contains("lldb version") )
            break;
    }
    data.replace("\r\n", "\n");
    QByteArray addr, off, size;
    QByteArrayList lines = data.split('\n');

    /*
    (QByteArray::Data *) hello.d = 0x00000000005d9ee0
    (QAtomicOps<int>::Type) hello.d->ref.atomic._q_value = 1
    (int) hello.d->size = 5
    (uint:31) hello.d->alloc = 6
    (uint:1) hello.d->capacityReserved = 0
    (qptrdiff) hello.d->offset = 24
    */
    foreach( const QByteArray& line, lines )
    {
        if( line.startsWith("(QByteArray::Data *)") )
            addr = line.mid(line.indexOf('=') + 1).trimmed();
        else if( line.startsWith("(qptrdiff)") )
            off = line.mid(line.indexOf('=') + 1).trimmed();
        else if( line.startsWith("(int)") )
            size = line.mid(line.indexOf('=') + 1).trimmed();
    }

    data.clear();

    if( size.toInt() > 0 )
    {
        // read memory:
        m_lldb.write("memory read -s1 -fy -c " + size + " -l " + size + " " + addr + "+" + off + " --force\n");
        m_lldb.write("version\n");
        while( true )
        {
            if( !m_lldb.waitForReadyRead() )
                break;
            const QByteArray out = m_lldb.readAllStandardOutput();
            const int colon = out.indexOf(':');
            if( colon != -1 )
            {
                const int nl = out.indexOf('\n',colon+1);
                data = out.mid(colon+1, nl - colon - 1);
            }
            if( out.contains("lldb version") )
                break;
        }
        data = QByteArray::fromHex(data);
    }

    connect(&m_lldb, SIGNAL(readyReadStandardOutput()), this, SLOT(readLldbStandardOutput()));
    return data;
}

QString LldbEngine2::fetchQString(const QByteArray &desig)
{
    disconnect(&m_lldb, SIGNAL(readyReadStandardOutput()), this, SLOT(readLldbStandardOutput()));
    m_lldb.write("frame variable " + desig + " -F -T -R -P1\n");
    m_lldb.write("version\n");
    QByteArray data;
    while( true )
    {
        if( !m_lldb.waitForReadyRead() )
            break;
        const QByteArray out = m_lldb.readAllStandardOutput();
        data += out;
        if( out.contains("lldb version") )
            break;
    }
    data.replace("\r\n", "\n");
    QByteArray addr, off, size;
    QByteArrayList lines = data.split('\n');

    /*
        (QString::Data *) str.d = 0x00000000005d9f10
        (QAtomicOps<int>::Type) str.d->ref.atomic._q_value = 1
        (int) str.d->size = 6
        (uint:31) str.d->alloc = 13
        (uint:1) str.d->capacityReserved = 0
        (qptrdiff) str.d->offset = 24
    */
    foreach( const QByteArray& line, lines )
    {
        if( line.startsWith("(QString::Data *)") )
            addr = line.mid(line.indexOf('=') + 1).trimmed();
        else if( line.startsWith("(qptrdiff)") )
            off = line.mid(line.indexOf('=') + 1).trimmed();
        else if( line.startsWith("(int)") )
            size = line.mid(line.indexOf('=') + 1).trimmed();
    }

    data.clear();

    const int s = size.toInt();
    if( s > 0 )
    {
        const QByteArray s2 = QByteArray::number(s*2);
        // read memory:
        m_lldb.write("memory read -s1 -fy -c " + s2 + " -l " + s2 + " " + addr + "+" + off + " --force\n");
        m_lldb.write("version\n");
        while( true )
        {
            if( !m_lldb.waitForReadyRead() )
                break;
            const QByteArray out = m_lldb.readAllStandardOutput();
            const int colon = out.indexOf(':');
            if( colon != -1 )
            {
                const int nl = out.indexOf('\n',colon+1);
                data = out.mid(colon+1, nl - colon - 1);
            }
            if( out.contains("lldb version") )
                break;
        }
        data = QByteArray::fromHex(data);
    }

    connect(&m_lldb, SIGNAL(readyReadStandardOutput()), this, SLOT(readLldbStandardOutput()));
    return QString::fromUtf16((const ushort*)data.constData(),s);
}

QByteArray LldbEngine2::fetchCString(const QByteArray &desig)
{
    disconnect(&m_lldb, SIGNAL(readyReadStandardOutput()), this, SLOT(readLldbStandardOutput()));
    m_lldb.write("frame variable " + desig + "\n");
    m_lldb.write("version\n");
    QByteArray data;
    while( true )
    {
        if( !m_lldb.waitForReadyRead() )
            break;
        const QByteArray out = m_lldb.readAllStandardOutput();
        data += out;
        if( out.contains("lldb version") )
            break;
    }
    data.replace("\r\n", "\n");
    QByteArrayList lines = data.split('\n');

    QByteArray res;
    foreach( const QByteArray& line, lines )
    {
        if( line.startsWith("(const char *)") || line.startsWith("(char *)") )
        {
            const int quote = line.indexOf('\"');
            res = line.mid(quote);
        }
    }

    connect(&m_lldb, SIGNAL(readyReadStandardOutput()), this, SLOT(readLldbStandardOutput()));
    return res;
}

void LldbEngine2::handleResponse(const QByteArray &data)
{
    QByteArrayList lines = data.split('\n');

    while( !lines.isEmpty() )
    {
        const QByteArray first = lines.first().trimmed();
        if( first.isEmpty() )
            lines.pop_front();
        else if( first.startsWith("Process ") )
            updateProcStat(lines);
        else if( first.startsWith("Breakpoint ") )
            updateBreakpoint(lines);
        else if( first.startsWith("Resuming thread") )
            lines.pop_front();
        else if( first.startsWith("* thread") )
            updateStack(lines);
        else if( first.startsWith("Current executable set to") )
        {
            notifyInferiorSetupOk(); // causes runEngine
            lines.pop_front();
        }else if( first.startsWith("(lldb)") )
            lines.pop_front();
        else if( first.startsWith('(') )
            updateVar(lines);
        else if( first.startsWith('}') )
        {
            QByteArrayList parts = d_curDesig.split('.');
            parts.pop_back();
            d_curDesig = parts.join('.');
            lines.pop_front();
        }
        else
        {
            // qDebug() << "*** LLDB stdout unhandled:" << first;
            lines.pop_front();
        }
    }
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
    showMessage(QString::fromUtf8(out), LogOutput);

    switch( state() )
    {
    case InferiorSetupRequested:
    case InferiorSetupOk:
    case InferiorRunRequested:
    case InferiorRunOk:
    case InferiorStopRequested:
    case InferiorStopOk:
    case InferiorShutdownRequested:
        break;
    default:
        qDebug() << "*** LLDB stdout ignored:" << out;
        return;
    }

    emit outputReady(out);
}

void LldbEngine2::readLldbStandardError()
{
    QByteArray err = m_lldb.readAllStandardError();
    qDebug() << "*** LLDB stderr:" << err;
    showMessage("Lldb stderr: " + QString::fromUtf8(err), LogError);
}

void LldbEngine2::fetchFullBacktrace()
{
    // TODO
    qWarning() << "LldbEngine2::fetchFullBacktrace not yet implemented";
}

void LldbEngine2::stdoutReady()
{
    if( d_out == 0 )
        return;
    const QByteArray str = d_out->readAll();
    if( str.isEmpty() )
        return;
    showMessage(QString::fromUtf8(str), AppOutput);
}

void LldbEngine2::stderrReady()
{
    if( d_err == 0 )
        return;
    const QByteArray str = d_err->readAll();
    if( str.isEmpty() )
        return;
    showMessage(QString::fromUtf8(str), AppError);
}


} // namespace Internal
} // namespace Debugger
