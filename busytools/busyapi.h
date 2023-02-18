#ifndef BUSY_API
#define BUSY_API

/*
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
*/

#include <QObject>
#include <QProcessEnvironment>
#include <QAbstractItemModel>
#include <QSet>
#include <projectexplorer/abi.h>
#include <QThread>

namespace busy
{

class Engine;

namespace Internal
{
    class ModuleImp;
    class ProjectImp;
    class ProductImp;
}

class CodeLocation
{
public:
    CodeLocation():d_row(0),d_col(0){}
    bool operator==(const CodeLocation& rhs) const { return d_path == rhs.d_path &&
                d_row == rhs.d_row && d_col == rhs.d_col; }

    QString filePath() const { return d_path; }
    int line() const { return d_row; }
    int column() const { return d_col; }

    QString d_path;
    int d_row, d_col;
};

class ErrorItem
{
public:
    QString description() const { return d_msg; }
    CodeLocation codeLocation() const { return d_loc; }

    QString d_msg;
    CodeLocation d_loc;
};

class ErrorInfo
{
public:
    QList<ErrorItem> items() const { return d_errs; }
    QString toString() const;
    bool hasError() const { return !d_errs.isEmpty(); }

    QList<ErrorItem> d_errs;
};

class ProcessResult
{
public:
    bool success;
    QString executableFilePath;
    QStringList arguments;
    QString workingDirectory;
    QStringList stdOut;
    QStringList stdErr;
    ProcessResult():success(true){}
};

class AbstractJob : public QThread
{
    Q_OBJECT
public:
    AbstractJob(QObject* owner):QThread(owner) {}

    ErrorInfo error() const { return err; }

signals:
    void taskStarted(const QString& description,int maxValue);
    void taskProgress(int curValue);
    void taskFinished(bool success);

public slots:
    virtual void cancel() {}

protected:
    ErrorInfo err;
};

enum CommandEchoMode {
    CommandEchoModeSilent,
    CommandEchoModeSummary,
    CommandEchoModeCommandLine,
    CommandEchoModeLast = CommandEchoModeCommandLine
};

class BuildOptions
{
public:
    void setFilesToConsider(const QStringList &files) {}

    void setChangedFiles(const QStringList &changedFiles) {}

    void setActiveFileTags(const QStringList &fileTags) {}

    static int defaultMaxJobCount() { return 0; }
    int maxJobCount() const { return 0; }
    void setMaxJobCount(int jobCount) {}

    bool dryRun() const { return false; }
    void setDryRun(bool dryRun) {}

    bool keepGoing() const { return false; }
    void setKeepGoing(bool keepGoing) {}

    CommandEchoMode echoMode() const { return CommandEchoModeSilent; }
    void setEchoMode(CommandEchoMode echoMode) {}

    bool install() const { return false; }
    void setInstall(bool install) {}

    bool removeExistingInstallation() const { return false; }
    void setRemoveExistingInstallation(bool removeExisting) {}
};

class CleanOptions
{
public:
    enum CleanType { CleanupAll, CleanupTemporaries };
    CleanType cleanType() const { return CleanupAll; }
    void setCleanType(CleanType cleanType) {}

    bool dryRun() const { return false; }
    void setDryRun(bool dryRun) {}
    void setKeepGoing(bool keepGoing) {}
    bool keepGoing() const { return false; }
};

class PropertyMap
{
public:
    enum Property {
        CXXFLAGS,
        CFLAGS,
        DEFINES,
        INCLUDEPATHS,
        SYSTEM_INCLUDEPATHS,
        FRAMEWORKPATHS,
        SYSTEM_FRAMEWORKPATHS,
        PRECOMPILEDHEADER,
        MaxProperty
    };
    QStringList properties[MaxProperty];
};

enum LoggerLevel
{
    LoggerMinLevel,
    LoggerError = LoggerMinLevel,
    LoggerWarning,
    LoggerInfo,
    LoggerDebug,
    LoggerTrace,
    LoggerMaxLevel = LoggerTrace
};

class ILogSink
{
    Q_DISABLE_COPY(ILogSink)
public:
    ILogSink();
    virtual ~ILogSink();

    void setLogLevel(LoggerLevel level){ d_level = level; }
    bool willPrint(LoggerLevel level) const { return level <= d_level; }
    void printWarning(const ErrorInfo &warning);
    void printMessage(LoggerLevel level, const QString &message, const QString &tag = QString(),bool force = false);

    static QString logLevelTag(LoggerLevel level);
protected:
    virtual void doPrintWarning(const ErrorInfo &warning) = 0;
    virtual void doPrintMessage(LoggerLevel level, const QString &message, const QString &tag) = 0;
private:
    LoggerLevel d_level;
};

class InstallableFile
{
public:
    QString sourceFilePath() const { return QString(); }
    QString targetFilePath() const { return QString(); }
    bool isExecutable() const { return false; }
};

class InstallOptions
{
public:
    static QString defaultInstallRoot() { return QString(); }

    QString installRoot() const { return QString(); }
    void setInstallRoot(const QString &installRoot) {}

    bool removeExistingInstallation() const { return false; }
    void setRemoveExistingInstallation(bool removeExisting) {}

    bool dryRun() const { return false; }
    void setDryRun(bool dryRun) {}

    bool keepGoing() const { return false; }
    void setKeepGoing(bool keepGoing) {}
};

class Settings
{
public:
    Settings(const QString &baseDir) {}

    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const { return QVariant(); }
    void setValue(const QString &key, const QVariant &value) {}
    void remove(const QString &key) {}
    QString baseDirectoy() const { return QString(); }
};

class TargetArtifact
{
public:
    TargetArtifact(const QString& path = QString(), bool exe = true):
        filePath(path),isExecutable(exe) {}

    bool isValid() const { return !filePath.isEmpty(); }

    QString filePath;
    bool isExecutable;
};

class Product
{
public:
    Product(Engine*,int id);
    Product();
    Product(const Product &other);
    Product &operator=(const Product &other);
    bool operator==( const Product& ) const;
    ~Product();

    bool isValid() const;

    QString name(bool altName = false) const;
    QString qualident() const;
    CodeLocation location() const;
    QList<TargetArtifact> targetArtifacts() const;
    QVariantMap properties() const;
    bool isEnabled() const;
    bool isRunnable() const;
    QStringList allFilePaths() const;
    PropertyMap buildConfig() const;
    QString executable(bool synthIfEmpty = true) const;
private:
    friend class Internal::ProductImp;
    QExplicitlySharedDataPointer<Internal::ProductImp> d_imp;
};

class RuleCommand
{
public:
    enum Type { ProcessCommandType, JavaScriptCommandType, InvalidType };
    Type type() const { return InvalidType; }
    QStringList arguments() const { return QStringList(); }
};

typedef QList<RuleCommand> RuleCommandList;

class BuildJob;
class CleanJob;
class InstallJob;

class Module // originally Qbs Project
{
public:
    Module(Engine*,int id);
    Module();
    Module(const Module &other);
    Module &operator=(const Module &other);
    ~Module();

    bool isValid() const;

    QString name() const;
    CodeLocation location() const; // path to the submod declaration
    QString busyFile() const;
    QString buildDirectory() const;
    QList<Product> products() const;
    QList<Module> subModules() const;

private:
    friend class Internal::ModuleImp;
    QExplicitlySharedDataPointer<Internal::ModuleImp> d_imp;
};

class SetupProjectParameters
{
public:
    QString projectFilePath;
    QString buildDir;
    QList<QPair<QByteArray,QByteArray> > params;
    QByteArrayList targets;
    QProcessEnvironment env;
    QString buildVariant;
    QString compilerCommand;
    QString toolchain;
    ProjectExplorer::Abi abi;
};

class Project
{
public:
    Project(const QString& path);
    Project();
    Project(const Project &other);
    Project &operator=(const Project &other);
    ~Project();

    bool isValid() const;
    Engine* getEngine() const;

    bool parse(const SetupProjectParameters &parameters, ILogSink *logSink);

    ErrorInfo errors() const;

    Module topModule() const;
    QList<Product> allProducts(bool onlyRunnables = false, bool onlyActives = false) const;
    QSet<QString> buildSystemFiles() const;

    QString targetExecutable(const Product &product, const InstallOptions &installoptions) const;
    QProcessEnvironment getRunEnvironment(const Product &product,
            const InstallOptions &installOptions,
            const QProcessEnvironment &environment, Settings *settings) const;

    enum ProductSelection { ProductSelectionDefaultOnly, ProductSelectionWithNonDefault };
    BuildJob *buildAllProducts(const BuildOptions &options,
                               ProductSelection productSelection = ProductSelectionDefaultOnly,
                               QObject *jobOwner = 0) const;
    BuildJob *buildSomeProducts(const QList<Product> &products, const BuildOptions &options,
                                QObject *jobOwner = 0) const;

    CleanJob *cleanAllProducts(const CleanOptions &options, QObject *jobOwner = 0) const { return 0; }

    InstallJob *installAllProducts(const InstallOptions &options,
                                   ProductSelection productSelection = ProductSelectionDefaultOnly,
                                   QObject *jobOwner = 0) const { return 0; }

    QList<InstallableFile> installableFilesForProject(const Module &project,
                                   const InstallOptions &options) const { return QList<InstallableFile>(); }

    QStringList generatedFiles(const Product &product, const QString &file,
                               const QStringList &tags = QStringList()) const { return QStringList(); }

    RuleCommandList ruleCommands(const Product &product, const QString &inputFilePath,
                         const QString &outputFileTag, ErrorInfo *error = 0) const { return RuleCommandList(); }

    ErrorInfo addFiles(const Product &product, const QStringList &filePaths) { return ErrorInfo(); }
    ErrorInfo removeFiles(const Product &product, const QStringList &filePaths) { return ErrorInfo(); }
private:
    friend class Internal::ProjectImp;
    QExplicitlySharedDataPointer<Internal::ProjectImp> d_imp;
};

class BuildJob : public AbstractJob
{
    Q_OBJECT
public:
    BuildJob(QObject* owner, const QByteArrayList&, const QProcessEnvironment& );
    ~BuildJob();

    void cancel();

signals:
    void reportCommandDescription(const QString& highlight, const QString& message);
    void reportProcessResult(const busy::ProcessResult&);

protected:
    void run();

private:
    QByteArrayList d_todo;
    QProcessEnvironment d_env;
    int d_cur;
    bool d_cancel;
};

class CleanJob : public AbstractJob
{
public:
};

class InstallJob : public AbstractJob
{
public:
};

class Profile
{
public:
    explicit Profile(const QString &name, Settings *settings) {}
    void setValue(const QString &key, const QVariant &value) {}
    void removeProfile() {}
};

}

Q_DECLARE_METATYPE(busy::ProcessResult)

#endif
