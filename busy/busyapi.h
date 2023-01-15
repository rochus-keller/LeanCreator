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

namespace busy
{

namespace Internal
{
    class ModuleImp;
}

// TODO: we don't need profiles and groups

class CodeLocation
{
public:
    QString filePath() const { return QString(); }
    int line() const { return 0; }
    int column() const { return 0; }
    bool operator==(const CodeLocation& rhs) const { return false; }
};

class ErrorItem
{
public:
    QString description() const { return QString(); }
    CodeLocation codeLocation() const { return CodeLocation(); }
    QString toString() const { return QString(); }
};

class ErrorInfo
{
public:
    QList<ErrorItem> items() const { return QList<ErrorItem>(); }
    QString toString() const { return QString(); }
    bool hasError() const { return false; }
};

class AbstractJob : public QObject
{
    Q_OBJECT
public:

    ErrorInfo error() const { return ErrorInfo(); }
public slots:
    void cancel() {}
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

inline QString canonicalArchitecture(const QString &architecture) { return QString(); }

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
    QStringList getModulePropertiesAsStringList(const QString &moduleName, const QString &propertyName) const { return QStringList(); }
    QVariant getModuleProperty(const QString &moduleName, const QString &propertyName) const { return QVariant(); }
};

class SourceArtifact
{
public:
    bool isValid() const { return false; }

    QString filePath() const { return QString(); }
    QStringList fileTags() const { return QStringList(); }
};

class GroupData
{
public:
    bool isValid() const { return false; }

    CodeLocation location() const { return CodeLocation(); }
    QString name() const { return QString(); }
    QList<SourceArtifact> allSourceArtifacts() const { return QList<SourceArtifact>(); }
    PropertyMap properties() const { return PropertyMap(); }
    bool isEnabled() const { return false; }
    QStringList allFilePaths() const { return QStringList(); }
    bool operator==(const GroupData&) const { return false; }
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
    ILogSink() {}
    void setLogLevel(LoggerLevel level) {}
};

inline QString logLevelTag(LoggerLevel level) { return QString(); }

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

class Preferences
{
public:
    explicit Preferences(Settings *settings, const QString &profileName = QString()) {}
    QStringList searchPaths(const QString &baseDir = QString()) const { return QStringList(); }
    QStringList pluginPaths(const QString &baseDir = QString()) const { return QStringList(); }
};

class ProcessResult
{
public:
    bool success() const { return false; }
    QString executableFilePath() const { return QString(); }
    QStringList arguments() const { return QStringList(); }
    QString workingDirectory() const { return QString(); }
    QStringList stdOut() const { return QStringList(); }
    QStringList stdErr() const { return QStringList(); }
};

class TargetArtifact
{
public:
    bool isValid() const { return false; }

    QString filePath() const { return QString(); }
    bool isExecutable() const { return false; }
};

class ProductData
{
public:
    bool isValid() const { return false; }

    QString name() const { return QString(); }
    QString profile() const { return QString(); }
    CodeLocation location() const { return CodeLocation(); }
    QList<TargetArtifact> targetArtifacts() const { return QList<TargetArtifact>(); }
    QList<GroupData> groups() const { return QList<GroupData>(); }
    QVariantMap properties() const { return QVariantMap(); }
    bool isEnabled() const { return false; }
    bool isRunnable() const { return false; }
    bool operator==( const ProductData& ) const { return false; }
};

class ModuleData
{
public:
    bool isValid() const { return false; }

    QString name() const { return QString(); }
    CodeLocation location() const { return CodeLocation(); }
    QString buildDirectory() const { return QString(); }
    QList<ProductData> products() const { return QList<ProductData>(); }
    QList<ModuleData> subModules() const { return QList<ModuleData>(); }
    QList<ProductData> allProducts() const { return QList<ProductData>(); }
    bool operator!=( const ModuleData& ) const { return false; }
};

class RunEnvironment
{
public:
    const QProcessEnvironment runEnvironment() const { return QProcessEnvironment(); }
};

class RuleCommand
{
public:
    enum Type { ProcessCommandType, JavaScriptCommandType, InvalidType };
    Type type() const { return InvalidType; }
    QStringList arguments() const { return QStringList(); }
};

typedef QList<RuleCommand> RuleCommandList;

class SetupProjectParameters
{
public:
    void setProjectFilePath(const QString &projectFilePath) {}
    void setBuildRoot(const QString &buildRoot) {}
    void setSearchPaths(const QStringList &searchPaths) {}
    void setPluginPaths(const QStringList &pluginPaths) {}
    void setLibexecPath(const QString &libexecPath) {}
    void setSettingsDirectory(const QString &settingsBaseDir) {}
    void setOverriddenValues(const QVariantMap &values) {}
    void setIgnoreDifferentProjectFilePath(bool doIgnore) {}
    void setDryRun(bool dryRun) {}
    void setEnvironment(const QProcessEnvironment &env) {}
    void setTopLevelProfile(const QString &profile) {}
    void setBuildVariant(const QString &buildVariant) {}
};

class BuildJob;
class CleanJob;
class InstallJob;
class SetupProjectJob;

class Module // originally Qbs Project
{
public:
    Module();
    Module(const Module &other);
    Module &operator=(const Module &other);
    ~Module();

    SetupProjectJob *setupProject(const SetupProjectParameters &parameters,
                                  ILogSink *logSink, QObject *jobOwner) { return 0; }

    bool isValid() const { return false; }
    QString profile() const { return QString(); }
    ModuleData projectData() const { return ModuleData(); }
    QString targetExecutable(const ProductData &product,
                             const InstallOptions &installoptions) const { return QString(); }
    RunEnvironment getRunEnvironment(const ProductData &product,
            const InstallOptions &installOptions,
            const QProcessEnvironment &environment, Settings *settings) const { return RunEnvironment(); }

    enum ProductSelection { ProductSelectionDefaultOnly, ProductSelectionWithNonDefault };
    BuildJob *buildAllProducts(const BuildOptions &options,
                               ProductSelection productSelection = ProductSelectionDefaultOnly,
                               QObject *jobOwner = 0) const { return 0; }
    BuildJob *buildSomeProducts(const QList<ProductData> &products, const BuildOptions &options,
                                QObject *jobOwner = 0) const { return 0; }

    CleanJob *cleanAllProducts(const CleanOptions &options, QObject *jobOwner = 0) const { return 0; }

    InstallJob *installAllProducts(const InstallOptions &options,
                                   ProductSelection productSelection = ProductSelectionDefaultOnly,
                                   QObject *jobOwner = 0) const { return 0; }

    QList<InstallableFile> installableFilesForProject(const ModuleData &project,
                                   const InstallOptions &options) const { return QList<InstallableFile>(); }

    QStringList generatedFiles(const ProductData &product, const QString &file,
                               const QStringList &tags = QStringList()) const { return QStringList(); }

    QSet<QString> buildSystemFiles() const { return QSet<QString>(); }

    RuleCommandList ruleCommands(const ProductData &product, const QString &inputFilePath,
                         const QString &outputFileTag, ErrorInfo *error = 0) const { return RuleCommandList(); }

    ErrorInfo addFiles(const ProductData &product, const GroupData &group,
                       const QStringList &filePaths) { return ErrorInfo(); }
    ErrorInfo removeFiles(const ProductData &product, const GroupData &group,
                          const QStringList &filePaths) { return ErrorInfo(); }
private:
    friend class Internal::ModuleImp;
    QExplicitlySharedDataPointer<Internal::ModuleImp> d_imp;
};

class SetupProjectJob : public AbstractJob
{
public:
    Module project() const { return Module(); }
};

class BuildJob : public AbstractJob
{
public:
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

class SettingsModel : public QAbstractItemModel
{
public:
    SettingsModel(const QString &settingsDir, QObject *parent = 0) {}

    void setEditable(bool isEditable) {}
    void setAdditionalProperties(const QVariantMap &properties) {}
    void reload() {}
    int keyColumn() const { return 0; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const { return 0; }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const { return QVariant(); }
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const { return QModelIndex(); }
    QModelIndex parent(const QModelIndex &child) const { return QModelIndex(); }
    int columnCount(const QModelIndex &parent = QModelIndex()) const { return 0; }
};

}

#endif
