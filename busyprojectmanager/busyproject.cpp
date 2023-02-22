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

#include "busyproject.h"
extern "C" {
#include <bshost.h>
}

#include "busybuildconfiguration.h"
#include "busylogsink.h"
#include "busyprojectfile.h"
#include "busyprojectmanager.h"
#include "busyprojectmanagerconstants.h"
#include "busynodes.h"
#include "busyparser.h"

#include <core/documentmanager.h>
#include <core/icontext.h>
#include <core/id.h>
#include <core/icore.h>
#include <core/iversioncontrol.h>
#include <core/vcsmanager.h>
#include <core/messagemanager.h>
#include <core/progressmanager/progressmanager.h>
#include <cpptools/cppmodelmanager.h>
#include <projectexplorer/buildenvironmentwidget.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/headerpath.h>
#if 0
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/uicodemodelsupport.h>
#include <qmljstools/qmljsmodelmanager.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#endif
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <busytools/busyapi.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QVariantMap>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace BusyProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// Constants:
// --------------------------------------------------------------------

static const char CONFIG_CXXFLAGS[] = "cxxFlags";
static const char CONFIG_CFLAGS[] = "cFlags";
static const char CONFIG_DEFINES[] = "defines";
static const char CONFIG_INCLUDEPATHS[] = "includePaths";
static const char CONFIG_SYSTEM_INCLUDEPATHS[] = "systemIncludePaths";
static const char CONFIG_FRAMEWORKPATHS[] = "frameworkPaths";
static const char CONFIG_SYSTEM_FRAMEWORKPATHS[] = "systemFrameworkPaths";
static const char CONFIG_PRECOMPILEDHEADER[] = "precompiledHeader";

// --------------------------------------------------------------------
// BusyProject:
// --------------------------------------------------------------------

BusyProject::BusyProject(BusyManager *manager, const QString &fileName) :
    m_manager(manager),
    m_project(fileName),
    m_fileName(fileName),
    m_rootProjectNode(0),
    m_busyUpdateFutureInterface(0),
    m_parsingScheduled(false),
    m_cancelStatus(CancelStatusNone),
    m_currentBc(0)
{
    if( fileName.endsWith("BUSY") || fileName.endsWith("BUSY.busy") )
        m_projectName = QFileInfo(fileName).absoluteDir().dirName();
    else
        m_projectName = QFileInfo(fileName).baseName();

    m_parsingDelay.setInterval(1000); // delay parsing by 1s.

    setId(Constants::PROJECT_ID);
    setProjectContext(Context(Constants::PROJECT_ID));
    setProjectLanguages(Context(ProjectExplorer::Constants::LANG_CXX));

    connect(this, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
            this, SLOT(changeActiveTarget(ProjectExplorer::Target*)));
    connect(this, SIGNAL(addedTarget(ProjectExplorer::Target*)),
            this, SLOT(targetWasAdded(ProjectExplorer::Target*)));
    connect(this, SIGNAL(environmentChanged()), this, SLOT(delayParsing()));

    connect(&m_parsingDelay, SIGNAL(timeout()), this, SLOT(startParsing()));

    updateDocuments(QSet<QString>() << fileName);

    // NOTE: BusyProjectNode does not use this as a parent!
    m_rootProjectNode = new BusyRootProjectNode(this); // needs documents to be initialized!
}

BusyProject::~BusyProject()
{
    m_codeModelFuture.cancel();
    if (m_busyUpdateFutureInterface) {
        m_busyUpdateFutureInterface->reportCanceled();
        m_busyUpdateFutureInterface->reportFinished();
        delete m_busyUpdateFutureInterface;
        m_busyUpdateFutureInterface = 0;
    }

    // Deleting the root node triggers a few things, make sure rootProjectNode
    // returns 0 already
    BusyProjectNode *root = m_rootProjectNode;
    m_rootProjectNode = 0;
    delete root;
}

QString BusyProject::displayName() const
{
    return m_projectName;
}

IDocument *BusyProject::document() const
{
    foreach (IDocument *doc, m_busyDocuments) {
        if (doc->filePath().toString() == m_fileName)
            return doc;
    }
    QTC_ASSERT(false, return 0);
}

BusyManager *BusyProject::projectManager() const
{
    return m_manager;
}

ProjectNode *BusyProject::rootProjectNode() const
{
    return m_rootProjectNode;
}

static void collectFilesForProject(const busy::Module &project, QSet<QString> &result)
{
    result.insert(project.busyFile());
    foreach (const busy::Product &prd, project.products()) {
        foreach (const QString &file, prd.allFilePaths())
            result.insert(file);
        result.insert(prd.location().filePath());
    }
    foreach (const busy::Module &subProject, project.subModules())
        collectFilesForProject(subProject, result);
}

QStringList BusyProject::files(Project::FilesMode fileMode) const
{
    Q_UNUSED(fileMode);
    if (!m_rootModule.isValid() || isParsing())
        return QStringList();
    QSet<QString> result;
    collectFilesForProject(m_rootModule, result);
    result.unite(m_project.buildSystemFiles());
    return result.toList();
}

bool BusyProject::isProjectEditable() const
{
    return m_rootModule.isValid() && !isParsing() && !BuildManager::isBuilding();
}

class ChangeExpector
{
public:
    ChangeExpector(const QString &filePath, const QSet<IDocument *> &documents)
        : m_document(0)
    {
        foreach (IDocument * const doc, documents) {
            if (doc->filePath().toString() == filePath) {
                m_document = doc;
                break;
            }
        }
        QTC_ASSERT(m_document, return);
        DocumentManager::expectFileChange(filePath);
        m_wasInDocumentManager = DocumentManager::removeDocument(m_document);
        QTC_CHECK(m_wasInDocumentManager);
    }

    ~ChangeExpector()
    {
        QTC_ASSERT(m_document, return);
        DocumentManager::addDocument(m_document);
        DocumentManager::unexpectFileChange(m_document->filePath().toString());
    }

private:
    IDocument *m_document;
    bool m_wasInDocumentManager;
};

bool BusyProject::ensureWriteableBusyFile(const QString &file)
{
    // Ensure that the file is not read only
    QFileInfo fi(file);
    if (!fi.isWritable()) {
        // Try via vcs manager
        IVersionControl *versionControl =
            VcsManager::findVersionControlForDirectory(fi.absolutePath());
        if (!versionControl || !versionControl->vcsOpen(file)) {
            bool makeWritable = QFile::setPermissions(file, fi.permissions() | QFile::WriteUser);
            if (!makeWritable) {
                QMessageBox::warning(ICore::mainWindow(),
                                     tr("Failed!"),
                                     tr("Could not write project file %1.").arg(file));
                return false;
            }
        }
    }
    return true;
}

bool BusyProject::addFilesToProduct(BusyBaseProjectNode *node, const QStringList &filePaths,
        const busy::Product &productData, QStringList *notAdded)
{
    QTC_ASSERT(m_rootModule.isValid(), return false);
    QStringList allPaths = productData.allFilePaths();
    const QString productFilePath = productData.location().filePath();
    ChangeExpector expector(productFilePath, m_busyDocuments);
    ensureWriteableBusyFile(productFilePath);
    foreach (const QString &path, filePaths) {
        busy::ErrorInfo err = m_project.addFiles(productData, QStringList() << path);
        if (err.hasError()) {
            MessageManager::write(err.toString());
            *notAdded += path;
        } else {
            allPaths += path;
        }
    }
    if (notAdded->count() != filePaths.count()) {
        BusyGroupNode::setupFiles(node, productData,
                                 allPaths, QFileInfo(productFilePath).absolutePath(), true);
        m_rootProjectNode->update();
        emit fileListChanged();
    }
    return notAdded->isEmpty();
}

bool BusyProject::removeFilesFromProduct(BusyBaseProjectNode *node, const QStringList &filePaths,
        const busy::Product &productData,
        QStringList *notRemoved)
{
    QTC_ASSERT(m_rootModule.isValid(), return false);
    QStringList allPaths = productData.allFilePaths();
    const QString productFilePath = productData.location().filePath();
    ChangeExpector expector(productFilePath, m_busyDocuments);
    ensureWriteableBusyFile(productFilePath);
    foreach (const QString &path, filePaths) {
        busy::ErrorInfo err
                = m_project.removeFiles(productData, QStringList() << path);
        if (err.hasError()) {
            MessageManager::write(err.toString());
            *notRemoved += path;
        } else {
            allPaths.removeOne(path);
        }
    }
    if (notRemoved->count() != filePaths.count()) {
        BusyGroupNode::setupFiles(node, productData, allPaths,
                                 QFileInfo(productFilePath).absolutePath(), true);
        m_rootProjectNode->update();
        emit fileListChanged();
    }
    return notRemoved->isEmpty();
}

bool BusyProject::renameFileInProduct(BusyBaseProjectNode *node, const QString &oldPath,
        const QString &newPath, const busy::Product &productData)
{
    if (newPath.isEmpty())
        return false;
    QStringList dummy;
    if (!removeFilesFromProduct(node, QStringList() << oldPath, productData, &dummy))
        return false;
    busy::Product newProductData;
    foreach (const busy::Product &p, m_project.allProducts()) {
        if (uniqueProductName(p) == uniqueProductName(productData)) {
            newProductData = p;
            break;
        }
    }
    if (!newProductData.isValid())
        return false;

    return addFilesToProduct(node, QStringList() << newPath, newProductData, &dummy);
}

void BusyProject::invalidate()
{
    prepareForParsing();
}

busy::BuildJob *BusyProject::build(const busy::BuildOptions &opts, QStringList productNames,
                                 QString &error)
{
    QTC_ASSERT(busyProject().isValid(), return 0);
    QTC_ASSERT(!isParsing(), return 0);

#if 0
    // TODO: options: check for modified includes, use Ninja
    CppTools::CppModelManager *modelmanager = CppTools::CppModelManager::instance();
    CPlusPlus::Snapshot snap = modelmanager->snapshot();
    const Utils::FileNameList toTouch = snap.allFilesDependingOnModifieds();
    const QSet<QString> sources = m_project.allSources(true);
    QFile f("out.txt");
    f.open(QIODevice::WriteOnly);
    QTextStream out(&f);
    foreach( const Utils::FileName& f, toTouch )
        if( sources.contains(f.toString()) )
            out << f.toString() << endl;
    // here comes much more than expected.
    //    bs_touch2(f.toString().toUtf8().constData()); // TODO: don't directly touch if open
#endif
    busy::BuildOptions opts2 = opts;

    if (productNames.isEmpty())
        return busyProject().buildAllProducts(opts2);

    QList<busy::Product> products;
    foreach (const QString &productName, productNames) {
        bool found = false;
        foreach (const busy::Product &data, busyProject().allProducts()) {
            if (uniqueProductName(data) == productName) {
                found = true;
                products.append(data);
                break;
            }
        }
        if (!found) {
            error = tr("Cannot build: Selected products do not exist anymore.");
            return 0;
        }
    }

    return busyProject().buildSomeProducts(products, opts2);
}

busy::CleanJob *BusyProject::clean(const busy::CleanOptions &opts)
{
    if (!busyProject().isValid())
        return 0;
    return busyProject().cleanAllProducts(opts);
}

busy::InstallJob *BusyProject::install(const busy::InstallOptions &opts)
{
    if (!busyProject().isValid())
        return 0;
    return busyProject().installAllProducts(opts);
}

QString BusyProject::profileForTarget(const Target *t) const
{
    return m_manager->profileForKit(t->kit());
}

bool BusyProject::isParsing() const
{
    return m_busyUpdateFutureInterface;
}

bool BusyProject::hasParseResult() const
{
    return busyModule().isValid();
}

busy::Module BusyProject::busyModule() const
{
    return m_rootModule;
}

busy::Project BusyProject::busyProject() const
{
    return m_project;
}

bool BusyProject::needsSpecialDeployment() const
{
    return true;
}

void BusyProject::handleBusyParsingDone(bool success)
{
    const CancelStatus cancelStatus = m_cancelStatus;
    m_cancelStatus = CancelStatusNone;

    // Start a new one parse operation right away, ignoring the old result.
    if (cancelStatus == CancelStatusCancelingForReparse) {
        parseCurrentBuildConfiguration();
        return;
    }

    generateErrors(m_project.errors());

    bool dataChanged = false;
    if (true) { // NOTE: also show tree in case of parser error, not only if success is true
        m_rootModule = m_project.topModule();
        QTC_CHECK(m_rootModule.isValid());

        m_rootProjectNode->update();

        updateDocuments(m_project.isValid()
                        ? m_project.buildSystemFiles() : QSet<QString>() << m_fileName);
        dataChanged = true;
    }

    if (m_busyUpdateFutureInterface) {
        if( !success )
            m_busyUpdateFutureInterface->reportCanceled();
        m_busyUpdateFutureInterface->reportFinished();
        delete m_busyUpdateFutureInterface;
        m_busyUpdateFutureInterface = 0;
    }

    if (dataChanged) { // Do this now when isParsing() is false!
        updateCppCodeModel();
        //updateQmlJsCodeModel();
        updateBuildTargetData();

        emit fileListChanged();
    }
    emit projectParsingDone(success);
}

void BusyProject::targetWasAdded(Target *t)
{
    connect(t, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(delayParsing()));
    connect(t, SIGNAL(buildDirectoryChanged()), this, SLOT(delayParsing()));
}

void BusyProject::changeActiveTarget(Target *t)
{
    BuildConfiguration *bc = 0;
    if (t && t->kit())
        bc = t->activeBuildConfiguration();
    buildConfigurationChanged(bc);
}

void BusyProject::buildConfigurationChanged(BuildConfiguration *bc)
{
    if (m_currentBc)
        disconnect(m_currentBc, SIGNAL(busyConfigurationChanged()), this, SLOT(delayParsing()));

    m_currentBc = qobject_cast<BusyBuildConfiguration *>(bc);
    if (m_currentBc) {
        connect(m_currentBc, SIGNAL(busyConfigurationChanged()), this, SLOT(delayParsing()));
        delayParsing();
    } else {
        invalidate();
    }
}

void BusyProject::startParsing()
{
    // Busy does update the build graph during the build. So we cannot
    // start to parse while a build is running or we will lose information.
    if (BuildManager::isBuilding(this)) {
        scheduleParsing();
        return;
    }

    parseCurrentBuildConfiguration();
}

void BusyProject::delayParsing()
{
    m_parsingDelay.start();
}

void BusyProject::parseCurrentBuildConfiguration()
{
    m_parsingScheduled = false;
    if (m_cancelStatus == CancelStatusCancelingForReparse)
        return;

    // The CancelStatusCancelingAltoghether type can only be set by a build job, during
    // which no other parse requests come through to this point (except by the build job itself,
    // but of course not while canceling is in progress).
    QTC_ASSERT(m_cancelStatus == CancelStatusNone, return);

    if (!activeTarget())
        return;
    BusyBuildConfiguration *bc = qobject_cast<BusyBuildConfiguration *>(activeTarget()->activeBuildConfiguration());
    if (!bc)
        return;

    // New parse requests override old ones.
    // NOTE: We need to wait for the current operation to finish, since otherwise there could
    //       be a conflict. Consider the case where the old qbs::ProjectSetupJob is writing
    //       to the build graph file when the cancel request comes in. If we don't wait for
    //       acknowledgment, it might still be doing that when the new one already reads from the
    //       same file.
#if 0
    // parser is too fast to cancel:
    if (m_qbsProjectParser) {
        m_cancelStatus = CancelStatusCancelingForReparse;
        m_qbsProjectParser->cancel();
        return;
    }
#endif

    parse(bc->busyConfiguration(), bc->environment(), bc->buildDirectory().toString());
}

void BusyProject::cancelParsing()
{
    m_cancelStatus = CancelStatusCancelingAltoghether;
    // parser is too fast to cancel: m_qbsProjectParser->cancel();
}

void BusyProject::updateAfterBuild()
{
    QTC_ASSERT(m_rootModule.isValid(), return);
    updateBuildTargetData();
    updateCppCompilerCallData();
}

Project::RestoreResult BusyProject::fromMap(const QVariantMap &map, QString *errorMessage)
{
    RestoreResult result = Project::fromMap(map, errorMessage);
    if (result != RestoreResult::Ok)
        return result;

    Kit *defaultKit = KitManager::defaultKit();
    if (!activeTarget() && defaultKit) {
        Target *t = new Target(this, defaultKit);
        t->updateDefaultBuildConfigurations();
        t->updateDefaultDeployConfigurations();
        t->updateDefaultRunConfigurations();
        addTarget(t);
    }

    return RestoreResult::Ok;
}

void BusyProject::generateErrors(const busy::ErrorInfo &e)
{
    foreach (const busy::ErrorItem &item, e.items())
        TaskHub::addTask(Task::Error, item.description(),
                         ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM,
                         FileName::fromString(item.codeLocation().filePath()),
                         item.codeLocation().line());

}

QString BusyProject::productDisplayName(const busy::Project &project,
                                       const busy::Product &product)
{
    const QString displayName = product.name(true);
    return displayName;
}

QString BusyProject::uniqueProductName(const busy::Product &product)
{
    QString name = product.qualident();
    if( name.isEmpty() )
        name = product.name();
    return name;
}

void BusyProject::parse(const QVariantMap &config, const Environment &env, const QString &dir)
{
    prepareForParsing();

    m_parsingDelay.stop();

    ProjectExplorer::Kit *kit = activeTarget()->kit();
    Q_ASSERT(kit);

    BusyManager::instance()->updateProfileIfNecessary(kit);

    busy::SetupProjectParameters params;
    QVariantMap userConfig = config;
    params.buildDir = dir;
    QString specialKey = QLatin1String(Constants::BUSY_CONFIG_PROFILE_KEY);
    specialKey = QLatin1String(Constants::BUSY_CONFIG_VARIANT_KEY);
    params.buildVariant = userConfig.take(specialKey).toString();
    BusyParamParser::Result res1 = BusyParamParser::parse(
                userConfig.value(Constants::BUSY_CONFIG_PARAMS_KEY).toString());
    params.params = res1.d_params;
    BusyTargetParser::Result res2 = BusyTargetParser::parse(
                userConfig.value(Constants::BUSY_CONFIG_TARGETS_KEY).toString());
    params.targets = res2.d_targets;
    params.projectFilePath = projectFilePath().toString();

    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(kit);

    if( tc )
    {
        Core::Id id = tc->typeId();
        if( id == ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID )
            params.toolchain = "gcc";
        else if( id == ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID )
            params.toolchain = "clang";
        else if( id == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID )
            params.toolchain = "msvc";
        else if( id == ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID )
            params.toolchain = "gcc";
        params.compilerCommand = tc->compilerCommand().toString();
        params.abi = tc->targetAbi();
        Environment tmp = env;
        tc->addToEnvironment(tmp);
        params.env = tmp.toProcessEnvironment();
    }else
        params.env = env.toProcessEnvironment();

    const bool res = m_project.parse(params, BusyManager::logSink());
    //  void parse(const QVariantMap &config, const Utils::Environment &env, const QString &dir);
    emit projectParsingStarted();
    handleBusyParsingDone(res);
}

void BusyProject::prepareForParsing()
{
    TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
    if (m_busyUpdateFutureInterface) {
        m_busyUpdateFutureInterface->reportCanceled();
        m_busyUpdateFutureInterface->reportFinished();
    }
    delete m_busyUpdateFutureInterface;
    m_busyUpdateFutureInterface = 0;

    m_busyUpdateFutureInterface = new QFutureInterface<bool>();
    m_busyUpdateFutureInterface->setProgressRange(0, 0);
    ProgressManager::addTask(m_busyUpdateFutureInterface->future(),
        tr("Reading Project \"%1\"").arg(displayName()), "Busy.BusyEvaluate");
    m_busyUpdateFutureInterface->reportStarted();
}

void BusyProject::updateDocuments(const QSet<QString> &files)
{
    // Update documents:
    QSet<QString> newFiles = files;
    QTC_ASSERT(!newFiles.isEmpty(), newFiles << m_fileName);
    QSet<QString> oldFiles;
    foreach (IDocument *doc, m_busyDocuments)
        oldFiles.insert(doc->filePath().toString());

    QSet<QString> filesToAdd = newFiles;
    filesToAdd.subtract(oldFiles);
    QSet<QString> filesToRemove = oldFiles;
    filesToRemove.subtract(newFiles);

    QSet<IDocument *> currentDocuments = m_busyDocuments;
    foreach (IDocument *doc, currentDocuments) {
        if (filesToRemove.contains(doc->filePath().toString())) {
            m_busyDocuments.remove(doc);
            delete doc;
        }
    }
    QSet<IDocument *> toAdd;
    foreach (const QString &f, filesToAdd)
        toAdd.insert(new BusyProjectFile(this, f));

    DocumentManager::addDocuments(toAdd.toList());
    m_busyDocuments.unite(toAdd);
}

void BusyProject::updateCppCodeModel()
{
    if (!m_rootModule.isValid())
        return;


    CppTools::CppModelManager *modelmanager = CppTools::CppModelManager::instance();
    CppTools::ProjectInfo pinfo(this);
    CppTools::ProjectPartBuilder ppBuilder(pinfo);

    ppBuilder.setQtVersion(CppTools::ProjectPart::NoQt);

    QHash<QString, QString> uiFiles;
    foreach (const busy::Product &prd, m_project.allProducts()) {
        const busy::PropertyMap &props = prd.buildConfig();

        ppBuilder.setCxxFlags(props.properties[busy::PropertyMap::CXXFLAGS]);
        ppBuilder.setCFlags(props.properties[busy::PropertyMap::CFLAGS]);

        QStringList list = props.properties[busy::PropertyMap::DEFINES];
        QByteArray grpDefines;
        foreach (const QString &def, list) {
            QByteArray data = def.toUtf8();
            int pos = data.indexOf('=');
            if (pos >= 0)
                data[pos] = ' ';
            else
                data.append(" 1"); // cpp.defines: [ "FOO" ] is considered to be "FOO=1"
            grpDefines += (QByteArray("#define ") + data + '\n');
        }
        ppBuilder.setDefines(grpDefines);

        list = props.properties[busy::PropertyMap::INCLUDEPATHS];
        list.append(props.properties[busy::PropertyMap::SYSTEM_INCLUDEPATHS]);
        CppTools::ProjectPart::HeaderPaths grpHeaderPaths;
        foreach (const QString &p, list)
            grpHeaderPaths += CppTools::ProjectPart::HeaderPath(
                        FileName::fromUserInput(p).toString(),
                        CppTools::ProjectPart::HeaderPath::IncludePath);

        list = props.properties[busy::PropertyMap::FRAMEWORKPATHS];
        list.append(props.properties[busy::PropertyMap::SYSTEM_FRAMEWORKPATHS]);
        foreach (const QString &p, list)
            grpHeaderPaths += CppTools::ProjectPart::HeaderPath(
                        FileName::fromUserInput(p).toString(),
                        CppTools::ProjectPart::HeaderPath::FrameworkPath);

        ppBuilder.setHeaderPaths(grpHeaderPaths);

        const QStringList pch = props.properties[busy::PropertyMap::PRECOMPILEDHEADER];
        ppBuilder.setPreCompiledHeaders(pch);

        ppBuilder.setDisplayName(prd.name(true));
        ppBuilder.setProjectFile(QString::fromLatin1("%1:%2:%3")
                .arg(prd.location().filePath())
                .arg(prd.location().line())
                .arg(prd.location().column()));

        const QStringList files = prd.allFilePaths();
        foreach (const QString &file, files) {
            if (file.endsWith(QLatin1String(".ui"))) {
                QStringList generated = m_rootProjectNode->busyProject()
                        .generatedFiles(prd, file, QStringList(QLatin1String("hpp")));
                if (generated.count() == 1)
                    uiFiles.insert(file, generated.at(0));
            }
        }

        const QList<Id> languages = ppBuilder.createProjectPartsForFiles(files);
        foreach (Id language, languages)
            setProjectLanguage(language, true);
    }

    pinfo.finish();

    // Update the code model
    m_codeModelFuture.cancel();
    m_codeModelFuture = modelmanager->updateProjectInfo(pinfo);
    m_codeModelProjectInfo = modelmanager->projectInfo(this);
    QTC_CHECK(m_codeModelProjectInfo == pinfo);
}

void BusyProject::updateCppCompilerCallData()
{
    CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    QTC_ASSERT(m_codeModelProjectInfo == modelManager->projectInfo(this), return);

    CppTools::ProjectInfo::CompilerCallData data;
    foreach (const busy::Product &product, m_project.allProducts()) {

        if (!product.isEnabled())
            continue;

        foreach (const QString &file, product.allFilePaths()) {
            if (!CppTools::ProjectFile::isSource(CppTools::ProjectFile::classify(file)))
                continue;

            busy::ErrorInfo errorInfo;
            const busy::RuleCommandList ruleCommands
                   = m_project.ruleCommands(product, file, QLatin1String("obj"), &errorInfo);
            if (errorInfo.hasError())
                continue;

            QList<QStringList> calls;
            foreach (const busy::RuleCommand &ruleCommand, ruleCommands) {
                if (ruleCommand.type() == busy::RuleCommand::ProcessCommandType)
                    calls << ruleCommand.arguments();
            }

            if (!calls.isEmpty())
                data.insert(file, calls);
        }
    }

    m_codeModelProjectInfo.setCompilerCallData(data);
    const QFuture<void> future = modelManager->updateProjectInfo(m_codeModelProjectInfo);
    QTC_CHECK(future.isFinished()); // No reparse of files expected
}

void BusyProject::updateApplicationTargets()
{
    BuildTargetInfoList applications;
    foreach (const busy::Product &product, m_project.allProducts(true,true)) {
        const QString displayName = productDisplayName(m_project, product);
        QList<busy::TargetArtifact> ta = product.targetArtifacts();
        if (ta.isEmpty()) { // No build yet.
            applications.list << BuildTargetInfo(displayName,
                    FileName(),
                    FileName::fromString(product.location().filePath()));
            continue;
        }
        foreach (const busy::TargetArtifact &ta, ta) {
            QTC_ASSERT(ta.isValid(), continue);
            if (!ta.isExecutable)
                continue;
            applications.list << BuildTargetInfo(displayName,
                    FileName::fromString(ta.filePath),
                    FileName::fromString(product.location().filePath()));
        }
    }
    activeTarget()->setApplicationTargets(applications);
}

void BusyProject::updateDeploymentInfo()
{
    DeploymentData deploymentData;
    if (m_rootModule.isValid()) {
        busy::InstallOptions installOptions;
        installOptions.setInstallRoot(QLatin1String("/"));
        foreach (const busy::InstallableFile &f, m_project
                     .installableFilesForProject(m_rootModule, installOptions)) {
            deploymentData.addFile(f.sourceFilePath(), QFileInfo(f.targetFilePath()).path(),
                    f.isExecutable() ? DeployableFile::TypeExecutable : DeployableFile::TypeNormal);
        }
    }
    activeTarget()->setDeploymentData(deploymentData);
}

void BusyProject::updateBuildTargetData()
{
    updateApplicationTargets();
    updateDeploymentInfo();
    foreach (Target *t, targets())
        t->updateDefaultRunConfigurations();
}

} // namespace Internal
} // namespace BusyProjectManager
