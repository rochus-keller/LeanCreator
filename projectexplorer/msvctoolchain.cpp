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

#include "msvctoolchain.h"

#include "msvcparser.h"
#include "projectexplorerconstants.h"

#include <utils/algorithm.h>
#include <utils/synchronousprocess.h>
#include <utils/winutils.h>
#include <utils/qtcassert.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <QLabel>
#include <QFormLayout>

#define KEY_ROOT "ProjectExplorer.MsvcToolChain."
static const char varsBatKeyC[] = KEY_ROOT"VarsBat";
static const char varsBatArgKeyC[] = KEY_ROOT"VarsBatArg";
static const char supportedAbiKeyC[] = KEY_ROOT"SupportedAbi";

enum { debug = 0 };

namespace ProjectExplorer {
namespace Internal {
using namespace Utils;
// --------------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------------

struct VisualStudioInstallation
{
    QString vsName;
    int majorVersion;
    QString path;       // Main installation path
    QString vcVarsPath; // Path under which the various vc..bat are to be found
    QString vcVarsAll;

    VisualStudioInstallation():majorVersion(0){}
};

struct MsvcPlatform
{
    MsvcToolChain::Platform platform;
    const char *name;
    const char *prefix; // VS up until 14.0 (MSVC2015)
    const char *bat;
};

const MsvcPlatform platforms[]
    = {{MsvcToolChain::x86, "x86", "/bin", "vcvars32.bat"},
       {MsvcToolChain::amd64, "amd64", "/bin/amd64", "vcvars64.bat"},
       {MsvcToolChain::x86_amd64, "x86_amd64", "/bin/x86_amd64", "vcvarsx86_amd64.bat"},
       {MsvcToolChain::ia64, "ia64", "/bin/ia64", "vcvars64.bat"},
       {MsvcToolChain::x86_ia64, "x86_ia64", "/bin/x86_ia64", "vcvarsx86_ia64.bat"},
       {MsvcToolChain::arm, "arm", "/bin/arm", "vcvarsarm.bat"},
       {MsvcToolChain::x86_arm, "x86_arm", "/bin/x86_arm", "vcvarsx86_arm.bat"},
       {MsvcToolChain::amd64_arm, "amd64_arm", "/bin/amd64_arm", "vcvarsamd64_arm.bat"},
       {MsvcToolChain::amd64_x86, "amd64_x86", "/bin/amd64_x86", "vcvarsamd64_x86.bat"},
       {MsvcToolChain::x86_arm64, "x86_arm64", "/bin/x86_arm64", "vcvarsx86_arm64.bat"},
       {MsvcToolChain::amd64_arm64, "amd64_arm64", "/bin/amd64_arm64", "vcvarsamd64_arm64.bat"},
       {MsvcToolChain::arm64, "arm64", "/bin/arm64", "vcvarsarm64.bat"},
       {MsvcToolChain::arm64_x86, "arm64_x86", "/bin/arm64_x86", "vcvarsarm64_x86.bat"},
       {MsvcToolChain::arm64_amd64, "arm64_amd64", "/bin/arm64_amd64", "vcvarsarm64_amd64.bat"}};

static const MsvcPlatform *platformEntryFromName(const QString &name)
{
    for (const MsvcPlatform &p : platforms) {
        if (name == QLatin1String(p.name))
            return &p;
    }
    return nullptr;
}

static const MsvcPlatform *platformEntry(MsvcToolChain::Platform t)
{
    for (const MsvcPlatform &p : platforms) {
        if (p.platform == t)
            return &p;
    }
    return nullptr;
}

static QString platformName(MsvcToolChain::Platform t)
{
    if (const MsvcPlatform *p = platformEntry(t))
        return QLatin1String(p->name);
    return QString();
}

static bool hostPrefersPlatform(MsvcToolChain::Platform platform)
{
    switch (HostOsInfo::hostArchitecture()) {
    case HostOsInfo::HostArchitectureAMD64:
        return platform == MsvcToolChain::amd64 || platform == MsvcToolChain::amd64_arm
               || platform == MsvcToolChain::amd64_x86 || platform == MsvcToolChain::amd64_arm64;
    case HostOsInfo::HostArchitectureX86:
        return platform == MsvcToolChain::x86 || platform == MsvcToolChain::x86_amd64
               || platform == MsvcToolChain::x86_ia64 || platform == MsvcToolChain::x86_arm
               || platform == MsvcToolChain::x86_arm64;
    case HostOsInfo::HostArchitectureArm:
        return platform == MsvcToolChain::arm;
#if 0
    case HostOsInfo::HostArchitectureArm64:
        return platform == MsvcToolChain::arm64
               || platform == MsvcToolChain::arm64_x86 || platform == MsvcToolChain::arm64_amd64;
#endif
    case HostOsInfo::HostArchitectureItanium:
        return platform == MsvcToolChain::ia64;
    default:
        return false;
    }
}

static bool hostSupportsPlatform(MsvcToolChain::Platform platform)
{
    if (hostPrefersPlatform(platform))
        return true;

    switch (HostOsInfo::hostArchitecture()) {
    // The x86 host toolchains are not the preferred toolchains on amd64 but they are still
    // supported by that host
    case HostOsInfo::HostArchitectureAMD64:
        return platform == MsvcToolChain::x86 || platform == MsvcToolChain::x86_amd64
               || platform == MsvcToolChain::x86_ia64 || platform == MsvcToolChain::x86_arm
               || platform == MsvcToolChain::x86_arm64;
#if 0
    // The Arm64 host can run the cross-compilers via emulation of x86 and amd64
    case HostOsInfo::HostArchitectureArm64:
        return platform == MsvcToolChain::x86_arm || platform == MsvcToolChain::x86_arm64
               || platform == MsvcToolChain::amd64_arm || platform == MsvcToolChain::amd64_arm64
               || platform == MsvcToolChain::x86 || platform == MsvcToolChain::x86_amd64
               || platform == MsvcToolChain::amd64 || platform == MsvcToolChain::amd64_x86;
#endif
    default:
        return false;
    }
}

static unsigned char wordWidthForPlatform(MsvcToolChain::Platform platform)
{
    switch (platform) {
    case ProjectExplorer::Internal::MsvcToolChain::x86:
    case ProjectExplorer::Internal::MsvcToolChain::arm:
    case ProjectExplorer::Internal::MsvcToolChain::x86_arm:
    case ProjectExplorer::Internal::MsvcToolChain::amd64_arm:
    case ProjectExplorer::Internal::MsvcToolChain::amd64_x86:
    case ProjectExplorer::Internal::MsvcToolChain::arm64_x86:
        return 32;
    case ProjectExplorer::Internal::MsvcToolChain::amd64:
    case ProjectExplorer::Internal::MsvcToolChain::x86_amd64:
    case ProjectExplorer::Internal::MsvcToolChain::ia64:
    case ProjectExplorer::Internal::MsvcToolChain::x86_ia64:
    case ProjectExplorer::Internal::MsvcToolChain::amd64_arm64:
    case ProjectExplorer::Internal::MsvcToolChain::x86_arm64:
    case ProjectExplorer::Internal::MsvcToolChain::arm64:
    case ProjectExplorer::Internal::MsvcToolChain::arm64_amd64:
        return 64;
    }

    return 0;
}

static Abi::Architecture archForPlatform(MsvcToolChain::Platform platform)
{
    switch (platform) {
    case ProjectExplorer::Internal::MsvcToolChain::x86:
    case ProjectExplorer::Internal::MsvcToolChain::amd64:
    case ProjectExplorer::Internal::MsvcToolChain::x86_amd64:
    case ProjectExplorer::Internal::MsvcToolChain::amd64_x86:
    case ProjectExplorer::Internal::MsvcToolChain::arm64_x86:
    case ProjectExplorer::Internal::MsvcToolChain::arm64_amd64:
        return Abi::X86Architecture;
    case ProjectExplorer::Internal::MsvcToolChain::arm:
    case ProjectExplorer::Internal::MsvcToolChain::x86_arm:
    case ProjectExplorer::Internal::MsvcToolChain::amd64_arm:
    case ProjectExplorer::Internal::MsvcToolChain::x86_arm64:
    case ProjectExplorer::Internal::MsvcToolChain::amd64_arm64:
    case ProjectExplorer::Internal::MsvcToolChain::arm64:
        return Abi::ArmArchitecture;
    case ProjectExplorer::Internal::MsvcToolChain::ia64:
    case ProjectExplorer::Internal::MsvcToolChain::x86_ia64:
        return Abi::ItaniumArchitecture;
    }

    return Abi::UnknownArchitecture;
}

static Abi findAbiOfMsvc(MsvcToolChain::Type type, MsvcToolChain::Platform platform, const QString &version)
{
    Abi::OSFlavor flavor = Abi::UnknownFlavor;

    QString msvcVersionString = version;
    if (type == MsvcToolChain::WindowsSDK) {
        if (version == QLatin1String("v7.0") || version.startsWith(QLatin1String("6.")))
            msvcVersionString = QLatin1String("9.0");
        else if (version == QLatin1String("v7.0A") || version == QLatin1String("v7.1"))
            msvcVersionString = QLatin1String("10.0");
    }
    if (msvcVersionString.startsWith(QLatin1String("17.")))
        flavor = Abi::WindowsMsvc2022Flavor;
    else if (msvcVersionString.startsWith(QLatin1String("16.")))
        flavor = Abi::WindowsMsvc2019Flavor;
    else if (msvcVersionString.startsWith(QLatin1String("15.")))
        flavor = Abi::WindowsMsvc2017Flavor;
    else if (msvcVersionString.startsWith(QLatin1String("14.")))
        flavor = Abi::WindowsMsvc2015Flavor;
    else if (msvcVersionString.startsWith(QLatin1String("12.")))
        flavor = Abi::WindowsMsvc2013Flavor;
    else if (msvcVersionString.startsWith(QLatin1String("11.")))
        flavor = Abi::WindowsMsvc2012Flavor;
    else if (msvcVersionString.startsWith(QLatin1String("10.")))
        flavor = Abi::WindowsMsvc2010Flavor;
    else if (msvcVersionString.startsWith(QLatin1String("9.")))
        flavor = Abi::WindowsMsvc2008Flavor;
    else
        flavor = Abi::WindowsMsvc2005Flavor;
    const Abi result = Abi(archForPlatform(platform), Abi::WindowsOS, flavor, Abi::PEFormat,
                           wordWidthForPlatform(platform));
    if (!result.isValid())
        qWarning("Unable to completely determine the ABI of MSVC version %s (%s).",
                 qPrintable(version), qPrintable(result.toString()));
    return result;
}

static QString generateDisplayName(const QString &name,
                                   MsvcToolChain::Type t,
                                   MsvcToolChain::Platform p)
{
    if (t == MsvcToolChain::WindowsSDK) {
        QString sdkName = name;
        sdkName += QString::fromLatin1(" (%1)").arg(platformName(p));
        return sdkName;
    }
    // Comes as "9.0" from the registry
    QString vcName = QLatin1String("Microsoft Visual C++ Compiler ");
    vcName += name;
    vcName += QString::fromLatin1(" (%1)").arg(platformName(p));
    return vcName;
}

static QByteArray msvcCompilationFile()
{
    static const char* macros[] = {"_ATL_VER", "_CHAR_UNSIGNED", "__CLR_VER",
                                   "__cplusplus_cli", "__COUNTER__", "__cplusplus",
                                   "_CPPLIB_VER", "_CPPRTTI", "_CPPUNWIND",
                                   "_DEBUG", "_DLL", "__FUNCDNAME__",
                                   "__FUNCSIG__", "__FUNCTION__", "_INTEGRAL_MAX_BITS",
                                   "_M_ALPHA", "_M_AAMD64", "_M_CEE", "_M_CEE_PURE",
                                   "_M_CEE_SAFE", "_M_IX86", "_M_IA64",
                                   "_M_IX86_FP", "_M_MPPC", "_M_MRX000",
                                   "_M_PPC", "_M_X64", "_MANAGED",
                                   "_MFC_VER", "_MSC_BUILD", "_MSC_EXTENSIONS",
                                   "_MSC_FULL_VER", "_MSC_VER", "__MSVC_RUNTIME_CHECKS",
                                   "_MT", "_NATIVE_WCHAR_T_DEFINED", "_OPENMP",
                                   "_VC_NODEFAULTLIB", "_WCHAR_T_DEFINED", "_WIN32",
                                   "_WIN32_WCE", "_WIN64", "_Wp64",
                                   "__DATE__", "__TIME__", "__TIMESTAMP__",
                                   0};
    QByteArray file = "#define __PPOUT__(x) V##x=x\n\n";
    for (int i = 0; macros[i] != 0; ++i) {
        const QByteArray macro(macros[i]);
        file += "#if defined(" + macro + ")\n__PPOUT__("
                + macro + ")\n#endif\n";
    }
    file += "\nvoid main(){}\n\n";
    return file;
}

// Run MSVC 'cl' compiler to obtain #defines.
QByteArray MsvcToolChain::msvcPredefinedMacros(const QStringList cxxflags,
                                               const Utils::Environment &env) const
{
    QByteArray predefinedMacros = AbstractMsvcToolChain::msvcPredefinedMacros(cxxflags, env);

    QStringList toProcess;
    foreach (const QString &arg, cxxflags) {
        if (arg.startsWith(QLatin1String("/D"))) {
            QString define = arg.mid(2);
            int pos = define.indexOf(QLatin1Char('='));
            if (pos < 0) {
                predefinedMacros += "#define ";
                predefinedMacros += define.toLocal8Bit();
                predefinedMacros += '\n';
            } else {
                predefinedMacros += "#define ";
                predefinedMacros += define.left(pos).toLocal8Bit();
                predefinedMacros += ' ';
                predefinedMacros += define.mid(pos + 1).toLocal8Bit();
                predefinedMacros += '\n';
            }
        } else if (arg.startsWith(QLatin1String("/U"))) {
            predefinedMacros += "#undef ";
            predefinedMacros += arg.mid(2).toLocal8Bit();
            predefinedMacros += '\n';
        } else if (arg.startsWith(QLatin1String("-I"))) {
            // Include paths should not have any effect on defines
        } else {
            toProcess.append(arg);
        }
    }

    Utils::TempFileSaver saver(QDir::tempPath() + QLatin1String("/envtestXXXXXX.cpp"));
    saver.write(msvcCompilationFile());
    if (!saver.finalize()) {
        qWarning("%s: %s", Q_FUNC_INFO, qPrintable(saver.errorString()));
        return predefinedMacros;
    }
    QProcess cpp;
    cpp.setEnvironment(env.toStringList());
    cpp.setWorkingDirectory(QDir::tempPath());
    QStringList arguments;
    const Utils::FileName binary = env.searchInPath(QLatin1String("cl.exe"));
    if (binary.isEmpty()) {
        qWarning("%s: The compiler binary cl.exe could not be found in the path.", Q_FUNC_INFO);
        return predefinedMacros;
    }

    arguments << toProcess << QLatin1String("/EP") << QDir::toNativeSeparators(saver.fileName());
    cpp.start(binary.toString(), arguments);
    if (!cpp.waitForStarted()) {
        qWarning("%s: Cannot start '%s': %s", Q_FUNC_INFO, qPrintable(binary.toUserOutput()),
            qPrintable(cpp.errorString()));
        return predefinedMacros;
    }
    cpp.closeWriteChannel();
    if (!cpp.waitForFinished()) {
        Utils::SynchronousProcess::stopProcess(cpp);
        qWarning("%s: Timeout running '%s'.", Q_FUNC_INFO, qPrintable(binary.toUserOutput()));
        return predefinedMacros;
    }
    if (cpp.exitStatus() != QProcess::NormalExit) {
        qWarning("%s: '%s' crashed.", Q_FUNC_INFO, qPrintable(binary.toUserOutput()));
        return predefinedMacros;
    }

    const QList<QByteArray> output = cpp.readAllStandardOutput().split('\n');
    foreach (const QByteArray& line, output) {
        if (line.startsWith('V')) {
            QList<QByteArray> split = line.split('=');
            const QByteArray key = split.at(0).mid(1);
            QByteArray value = split.at(1);
            if (!value.isEmpty())
                value.chop(1); //remove '\n'
            predefinedMacros += "#define ";
            predefinedMacros += key;
            predefinedMacros += ' ';
            predefinedMacros += value;
            predefinedMacros += '\n';
        }
    }
    if (debug)
        qDebug() << "msvcPredefinedMacros" << predefinedMacros;
    return predefinedMacros;
}

// Windows: Expand the delayed evaluation references returned by the
// SDK setup scripts: "PATH=!Path!;foo". Some values might expand
// to empty and should not be added
static QString winExpandDelayedEnvReferences(QString in, const Utils::Environment &env)
{
    const QChar exclamationMark = QLatin1Char('!');
    for (int pos = 0; pos < in.size(); ) {
        // Replace "!REF!" by its value in process environment
        pos = in.indexOf(exclamationMark, pos);
        if (pos == -1)
            break;
        const int nextPos = in.indexOf(exclamationMark, pos + 1);
        if (nextPos == -1)
            break;
        const QString var = in.mid(pos + 1, nextPos - pos - 1);
        const QString replacement = env.value(var.toUpper());
        in.replace(pos, nextPos + 1 - pos, replacement);
        pos += replacement.size();
    }
    return in;
}

Utils::Environment MsvcToolChain::readEnvironmentSetting(Utils::Environment& env) const
{
    Utils::Environment result = env;
    if (!QFileInfo::exists(m_vcvarsBat))
        return result;

    QMap<QString, QString> envPairs;
    if (!generateEnvironmentSettings(env, m_vcvarsBat, m_varsBatArg, envPairs))
        return result;

    // Now loop through and process them
    QMap<QString,QString>::const_iterator envIter;
    for (envIter = envPairs.constBegin(); envIter!=envPairs.constEnd(); ++envIter) {
        const QString expandedValue = winExpandDelayedEnvReferences(envIter.value(), env);
        if (!expandedValue.isEmpty())
            result.set(envIter.key(), expandedValue);
    }

    if (debug) {
        const QStringList newVars = result.toStringList();
        const QStringList oldVars = env.toStringList();
        QDebug nsp = qDebug().nospace();
        foreach (const QString &n, newVars) {
            if (!oldVars.contains(n))
                nsp << n << '\n';
        }
    }
    return result;
}

// --------------------------------------------------------------------------
// MsvcToolChain
// --------------------------------------------------------------------------

MsvcToolChain::MsvcToolChain(const QString &name, const Abi &abi,
                             const QString &varsBat, const QString &varsBatArg, Detection d) :
    AbstractMsvcToolChain(Constants::MSVC_TOOLCHAIN_TYPEID, d, abi, varsBat),
    m_varsBatArg(varsBatArg)
{
    Q_ASSERT(!name.isEmpty());

    setDisplayName(name);
}

bool MsvcToolChain::isValid() const
{
    if (!AbstractMsvcToolChain::isValid())
        return false;
    QFileInfo fi(m_vcvarsBat);
    return fi.isFile() && fi.isExecutable();
}

MsvcToolChain::MsvcToolChain() :
    AbstractMsvcToolChain(Constants::MSVC_TOOLCHAIN_TYPEID, ManualDetection)
{
}

MsvcToolChain *MsvcToolChain::readFromMap(const QVariantMap &data)
{
    MsvcToolChain *tc = new MsvcToolChain;
    if (tc->fromMap(data))
        return tc;
    delete tc;
    return 0;
}

QString MsvcToolChain::typeDisplayName() const
{
    return MsvcToolChainFactory::tr("MSVC");
}

QList<Utils::FileName> MsvcToolChain::suggestedMkspecList() const
{
    // no longer required
    return QList<Utils::FileName>();
}

QVariantMap MsvcToolChain::toMap() const
{
    QVariantMap data = AbstractMsvcToolChain::toMap();
    data.insert(QLatin1String(varsBatKeyC), m_vcvarsBat);
    if (!m_varsBatArg.isEmpty())
        data.insert(QLatin1String(varsBatArgKeyC), m_varsBatArg);
    data.insert(QLatin1String(supportedAbiKeyC), m_abi.toString());
    return data;
}

bool MsvcToolChain::fromMap(const QVariantMap &data)
{
    if (!ToolChain::fromMap(data))
        return false;
    m_vcvarsBat = QDir::fromNativeSeparators(data.value(QLatin1String(varsBatKeyC)).toString());
    m_varsBatArg = data.value(QLatin1String(varsBatArgKeyC)).toString();
    const QString abiString = data.value(QLatin1String(supportedAbiKeyC)).toString();
    m_abi = Abi(abiString);

    return !m_vcvarsBat.isEmpty() && m_abi.isValid();
}


ToolChainConfigWidget *MsvcToolChain::configurationWidget()
{
    return new MsvcToolChainConfigWidget(this);
}

ToolChain *MsvcToolChain::clone() const
{
    return new MsvcToolChain(*this);
}

// --------------------------------------------------------------------------
// MsvcToolChainConfigWidget
// --------------------------------------------------------------------------

MsvcToolChainConfigWidget::MsvcToolChainConfigWidget(ToolChain *tc) :
    ToolChainConfigWidget(tc),
    m_varsBatDisplayLabel(new QLabel(this))
{
    m_mainLayout->addRow(new QLabel(tc->displayName()));
    m_varsBatDisplayLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_mainLayout->addRow(tr("Initialization:"), m_varsBatDisplayLabel);
    addErrorLabel();
    setFromToolChain();
}

void MsvcToolChainConfigWidget::setFromToolChain()
{
    MsvcToolChain *tc = static_cast<MsvcToolChain *>(toolChain());
    QTC_ASSERT(tc, return);
    QString varsBatDisplay = QDir::toNativeSeparators(tc->varsBat());
    if (!tc->varsBatArg().isEmpty()) {
        varsBatDisplay += QLatin1Char(' ');
        varsBatDisplay += tc->varsBatArg();
    }
    m_varsBatDisplayLabel->setText(varsBatDisplay);
}

// --------------------------------------------------------------------------
// MsvcToolChainFactory
// --------------------------------------------------------------------------

MsvcToolChainFactory::MsvcToolChainFactory()
{
    setDisplayName(tr("MSVC"));
}

bool MsvcToolChainFactory::checkForVisualStudioInstallation(const QString &vsName)
{
    const QSettings vsRegistry(
#ifdef Q_OS_WIN64
                QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\VisualStudio\\SxS\\VS7"),
#else
                QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\SxS\\VS7"),
#endif
                QSettings::NativeFormat);

    return vsRegistry.contains(vsName);
}

QString MsvcToolChainFactory::vcVarsBatFor(const QString &basePath, MsvcToolChain::Platform platform, bool allInOneFolder)
{
    QString result;
    if (const MsvcPlatform *p = platformEntry(platform)) {
        result += basePath;
        // Starting with 15.0 (MSVC2017), the .bat are in one folder.
        if ( !allInOneFolder )
            result += QLatin1String(p->prefix);
        result += QLatin1Char('/');
        result += QLatin1String(p->bat);
    }
    return result;
}

static ToolChain *findOrCreateToolChain(const QList<ToolChain *> &alreadyKnown,
                                        const QString &name, const Abi &abi,
                                        const QString &varsBat, const QString &varsBatArg,
                                        ToolChain::Detection d = ToolChain::ManualDetection)
{
    ToolChain *tc = Utils::findOrDefault(alreadyKnown,
                                         [&varsBat, &varsBatArg](ToolChain *tc) -> bool {
                                              if (tc->typeId() != Constants::MSVC_TOOLCHAIN_TYPEID)
                                                  return false;
                                              auto mtc = static_cast<MsvcToolChain *>(tc);
                                              return mtc->varsBat() == varsBat
                                                      && mtc->varsBatArg() == varsBatArg;
                                         });
    if (!tc)
        tc = new MsvcToolChain(name, abi, varsBat, varsBatArg, d);
    return tc;
}

static QString windowsProgramFilesDir()
{
#ifdef Q_OS_WIN64
    const char programFilesC[] = "ProgramFiles(x86)";
#else
    const char programFilesC[] = "ProgramFiles";
#endif
    return QDir::fromNativeSeparators(qgetenv(programFilesC));
}

// Detect build tools introduced with MSVC2015
static void detectCppBuildTools2015(QList<ToolChain *> *list)
{
    struct Entry {
        const char *postFix;
        const char *varsBatArg;
        Abi::Architecture architecture;
        Abi::BinaryFormat format;
        unsigned char wordSize;
    };

    const Entry entries[] = {{" (x86)", "x86", Abi::X86Architecture, Abi::PEFormat, 32},
                             {" (x64)", "amd64", Abi::X86Architecture, Abi::PEFormat, 64},
                             {" (x86_arm)", "x86_arm", Abi::ArmArchitecture, Abi::PEFormat, 32},
                             {" (x64_arm)", "amd64_arm", Abi::ArmArchitecture, Abi::PEFormat, 32},
                             {" (x86_arm64)", "x86_arm64", Abi::ArmArchitecture, Abi::PEFormat, 64},
                             {" (x64_arm64)", "amd64_arm64", Abi::ArmArchitecture, Abi::PEFormat, 64}};

    const QString name = "Microsoft Visual C++ Build Tools";
    const QString vcVarsBat = windowsProgramFilesDir() + '/' + name + "/vcbuildtools.bat";
    if (!QFileInfo(vcVarsBat).isFile())
        return;

    const size_t count = sizeof(entries) / sizeof(entries[0]);
    for (size_t i = 0; i < count; ++i) {
        const Entry &e = entries[i];

        const Abi abi(e.architecture, Abi::WindowsOS, Abi::WindowsMsvc2015Flavor,
                      e.format, e.wordSize);

        list->append(new MsvcToolChain(name + QLatin1String(e.postFix), abi,
                                       vcVarsBat, QLatin1String(e.varsBatArg),
                                       ToolChain::AutoDetection));
    }
}

// Detect build tools introduced with MSVC2017
static VisualStudioInstallation detectCppBuildTools2017()
{
    const QString installPath = windowsProgramFilesDir()
                                + "/Microsoft Visual Studio/2017/BuildTools";
    const QString vcVarsPath = installPath + "/VC/Auxiliary/Build";
    const QString vcVarsAllPath = vcVarsPath + "/vcvarsall.bat";

    VisualStudioInstallation installation;
    if (QFileInfo::exists(vcVarsAllPath))
    {
        installation.path = installPath;
        installation.vcVarsAll = vcVarsAllPath;
        installation.vcVarsPath = vcVarsPath;
        installation.majorVersion = 15;
        installation.vsName = "15.0";
    }
    return installation;
}

static QString fixRegistryPath(const QString &path)
{
    QString result = QDir::fromNativeSeparators(path);
    if (result.endsWith(QLatin1Char('/')))
        result.chop(1);
    return result;
}

static VisualStudioInstallation installationFromPathAndVersion(
    const QString &installationPath, int version)
{
    QString vcVarsPath = QDir::fromNativeSeparators(installationPath);
    if (!vcVarsPath.endsWith('/'))
        vcVarsPath += '/';
    if (version > 14)
        vcVarsPath += QStringLiteral("VC/Auxiliary/Build");
    else
        vcVarsPath += QStringLiteral("VC");

    VisualStudioInstallation installation;
    const QString vcVarsAllPath = vcVarsPath + QStringLiteral("/vcvarsall.bat");
    if (!QFileInfo(vcVarsAllPath).isFile()) {
        qWarning().noquote() << "Unable to find MSVC setup script "
                             << QDir::toNativeSeparators(vcVarsPath) << " in version " << version;
        return installation;
    }

    installation.path = installationPath;
    installation.majorVersion = version;
    installation.vsName = QString("%1.0").arg(version);
    installation.vcVarsPath = vcVarsPath;
    installation.vcVarsAll = vcVarsAllPath;
    return installation;
}

static QVector<VisualStudioInstallation> detectVisualStudioFromRegistry()
{
    QVector<VisualStudioInstallation> result;
#ifdef Q_OS_WIN64
    const QString keyRoot = QStringLiteral(
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\VisualStudio\\SxS\\");
#else
    const QString keyRoot = QStringLiteral(
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\SxS\\");
#endif
    QSettings vsRegistry(keyRoot + QStringLiteral("VS7"), QSettings::NativeFormat);
    QScopedPointer<QSettings> vcRegistry;
    const QStringList keys = vsRegistry.allKeys();
    for (const QString &vsName : keys) {
        const int dotPos = vsName.indexOf(QLatin1Char('.'));
        if (dotPos == -1)
            continue;
        const int version = vsName.left(dotPos).toInt();
        if (version) {
            const QString installationPath = fixRegistryPath(vsRegistry.value(vsName).toString());

            VisualStudioInstallation installation
                = installationFromPathAndVersion(installationPath, version);
            if (installation.majorVersion )
                result.append(installation);
        }
    }

    // Detect VS 2017 Build Tools
    auto installation = detectCppBuildTools2017();
    if (installation.majorVersion)
        result.append(installation);

    return result;
}

static QVector<VisualStudioInstallation> detectVisualStudioFromVsWhere(const QString &vswhere)
{
    QVector<VisualStudioInstallation> installations;
    Utils::QtcProcess vsWhereProcess;
    // TODO vsWhereProcess.setCodec(QTextCodec::codecForName("UTF-8"));
    const QString cmdArguments = "-products * -prerelease -legacy -format json -utf8";
    vsWhereProcess.setCommand(vswhere, cmdArguments);
    vsWhereProcess.start();

    if (!vsWhereProcess.waitForStarted()) {
        qWarning().noquote() << QDir::toNativeSeparators(vswhere) << "could not be started.";
        return installations;
    }
    if (!vsWhereProcess.waitForFinished(5000)) {
        qWarning("%s: Timeout running '%s'", Q_FUNC_INFO, qPrintable(vswhere));
        Utils::SynchronousProcess::stopProcess(vsWhereProcess);
        return installations;
    }
    if( vsWhereProcess.exitStatus() == QProcess::CrashExit )
    {
        qWarning().noquote().nospace()
            << QDir::toNativeSeparators(vswhere) << " crashed. Exit code: " << vsWhereProcess.exitCode();
        return installations;
    }

    QByteArray output = vsWhereProcess.readAllStandardOutput();
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(output, &error);
    if (error.error != QJsonParseError::NoError || doc.isNull()) {
        qWarning() << "Could not parse json document from vswhere output.";
        return installations;
    }

    const QJsonArray versions = doc.array();
    if (versions.isEmpty()) {
        qWarning() << "Could not detect any versions from vswhere output.";
        return installations;
    }

    for (const QJsonValue &vsVersion : versions) {
        const QJsonObject vsVersionObj = vsVersion.toObject();
        if (vsVersionObj.isEmpty()) {
            qWarning() << "Could not obtain object from vswhere version";
            continue;
        }

        QJsonValue value = vsVersionObj.value("installationVersion");
        if (value.isUndefined()) {
            qWarning() << "Could not obtain VS version from json output";
            continue;
        }
        const QString versionString = value.toString();
        const int dotPos = versionString.indexOf(QLatin1Char('.'));
        if (dotPos == -1)
            continue;
        const int version = versionString.left(dotPos).toInt();
        if( version == 0 )
            continue;
        value = vsVersionObj.value("installationPath");
        if (value.isUndefined()) {
            qWarning() << "Could not obtain VS installation path from json output";
            continue;
        }
        const QString installationPath = value.toString();
        VisualStudioInstallation installation
            = installationFromPathAndVersion(installationPath, version);

        if (installation.majorVersion)
            installations.append(installation);
    }
    return installations;
}

static QVector<VisualStudioInstallation> detectVisualStudio()
{
    const QString vswhere = windowsProgramFilesDir()
                            + "/Microsoft Visual Studio/Installer/vswhere.exe";
    if (QFileInfo::exists(vswhere)) {
        const QVector<VisualStudioInstallation> installations = detectVisualStudioFromVsWhere(vswhere);
        if (!installations.isEmpty())
            return installations;
    }

    return detectVisualStudioFromRegistry();
}

QList<ToolChain *> MsvcToolChainFactory::autoDetect(const QList<ToolChain *> &alreadyKnown)
{
    QList<ToolChain *> results;

    // 1) Installed SDKs preferred over standalone Visual studio
    const QSettings sdkRegistry(QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows"),
                                QSettings::NativeFormat);
    const QString defaultSdkPath = sdkRegistry.value(QLatin1String("CurrentInstallFolder")).toString();
    if (!defaultSdkPath.isEmpty())
    {
        foreach (const QString &sdkKey, sdkRegistry.childGroups())
        {
            const QString name = sdkRegistry.value(sdkKey + QLatin1String("/ProductName")).toString();
            const QString folder = sdkRegistry.value(sdkKey + QLatin1String("/InstallationFolder")).toString();
            if (folder.isEmpty())
                continue;

            QDir dir(folder);
            if (!dir.cd(QLatin1String("bin")))
                continue;
            QFileInfo fi(dir, QLatin1String("SetEnv.cmd"));
            if (!fi.exists())
                continue;

            QList<ToolChain *> tmp;
            tmp.append(findOrCreateToolChain(alreadyKnown,
                                             generateDisplayName(name, MsvcToolChain::WindowsSDK, MsvcToolChain::x86),
                                             findAbiOfMsvc(MsvcToolChain::WindowsSDK, MsvcToolChain::x86, sdkKey),
                                             fi.absoluteFilePath(), QLatin1String("/x86"), ToolChain::AutoDetection));
            // Add all platforms, cross-compiler is automatically selected by SetEnv.cmd if needed
            tmp.append(findOrCreateToolChain(alreadyKnown,
                                             generateDisplayName(name, MsvcToolChain::WindowsSDK, MsvcToolChain::amd64),
                                             findAbiOfMsvc(MsvcToolChain::WindowsSDK, MsvcToolChain::amd64, sdkKey),
                                             fi.absoluteFilePath(), QLatin1String("/x64"), ToolChain::AutoDetection));
            tmp.append(findOrCreateToolChain(alreadyKnown,
                                             generateDisplayName(name, MsvcToolChain::WindowsSDK, MsvcToolChain::ia64),
                                             findAbiOfMsvc(MsvcToolChain::WindowsSDK, MsvcToolChain::ia64, sdkKey),
                                             fi.absoluteFilePath(), QLatin1String("/ia64"), ToolChain::AutoDetection));
            tmp.append(findOrCreateToolChain(alreadyKnown,
                                             generateDisplayName(name, MsvcToolChain::WindowsSDK, MsvcToolChain::arm64),
                                             findAbiOfMsvc(MsvcToolChain::WindowsSDK, MsvcToolChain::arm64, sdkKey),
                                             fi.absoluteFilePath(), QLatin1String("/arm64"), ToolChain::AutoDetection));
            // Make sure the default is front.
            if (folder == defaultSdkPath)
                results = tmp + results;
            else
                results += tmp;
        } // foreach
    }

    // prioritized list.
    // x86_arm was put before amd64_arm as a workaround for auto detected windows phone
    // toolchains. As soon as windows phone builds support x64 cross builds, this change
    // can be reverted.
    const MsvcToolChain::Platform platforms[] = {MsvcToolChain::x86,
                                                 MsvcToolChain::amd64_x86,
                                                 MsvcToolChain::amd64,
                                                 MsvcToolChain::x86_amd64,
                                                 MsvcToolChain::arm,
                                                 MsvcToolChain::x86_arm,
                                                 MsvcToolChain::amd64_arm,
                                                 MsvcToolChain::x86_arm64,
                                                 MsvcToolChain::amd64_arm64,
                                                 MsvcToolChain::ia64,
                                                 MsvcToolChain::x86_ia64,
                                                 MsvcToolChain::arm64,
                                                 MsvcToolChain::arm64_x86,
                                                 MsvcToolChain::arm64_amd64};

    const QVector<VisualStudioInstallation> studios = detectVisualStudio();
    for (const VisualStudioInstallation &i : studios) {
        for (MsvcToolChain::Platform platform : platforms) {
            const bool toolchainInstalled
                = QFileInfo(vcVarsBatFor(i.vcVarsPath, platform, i.majorVersion >= 15 )).isFile();
            if (hostSupportsPlatform(platform) && toolchainInstalled) {
                results.append(
                    findOrCreateToolChain(alreadyKnown,
                                           generateDisplayName(i.vsName, MsvcToolChain::VS, platform),
                                           findAbiOfMsvc(MsvcToolChain::VS, platform, i.vsName),
                                           i.vcVarsAll,
                                           platformName(platform),
                                           ToolChain::AutoDetection ));
            }
        }
    }

    detectCppBuildTools2015(&results);

    return results;
}

bool MsvcToolChain::operator ==(const ToolChain &other) const
{
    if (!AbstractMsvcToolChain::operator ==(other))
        return false;
    const MsvcToolChain *msvcTc = static_cast<const MsvcToolChain *>(&other);
    return m_varsBatArg == msvcTc->m_varsBatArg;
}

bool MsvcToolChainFactory::canRestore(const QVariantMap &data)
{
    return typeIdFromMap(data) == Constants::MSVC_TOOLCHAIN_TYPEID;
}

} // namespace Internal
} // namespace ProjectExplorer
