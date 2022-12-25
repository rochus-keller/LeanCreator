/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2022 Rochus Keller (me@rochus-keller.ch) for LeanCreator
**
** This file is part of Qt Creator.
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppincludesfilter.h"

#include "cppmodelmanager.h"

#include <cplusplus/CppDocument.h>
#include <core/editormanager/documentmodel.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <QTimer>

using namespace Core;
using namespace CppTools;
using namespace CppTools::Internal;
using namespace ProjectExplorer;
namespace CppTools {
namespace Internal {

class CppIncludesIterator : public BaseFileFilter::Iterator
{
public:
    CppIncludesIterator(CPlusPlus::Snapshot snapshot, const QSet<QString> &seedPaths);

    void toFront();
    bool hasNext() const;
    QString next();
    QString filePath() const;
    QString fileName() const;

private:
    void fetchMore();

    CPlusPlus::Snapshot m_snapshot;
    QSet<QString> m_paths;
    QSet<QString> m_queuedPaths;
    QSet<QString> m_allResultPaths;
    QStringList m_resultQueue;
    QString m_currentPath;
};

} // Internal
} // CppTools


CppIncludesIterator::CppIncludesIterator(CPlusPlus::Snapshot snapshot,
                                         const QSet<QString> &seedPaths)
    : m_snapshot(snapshot),
      m_paths(seedPaths)
{
    toFront();
}

void CppIncludesIterator::toFront()
{
    m_queuedPaths = m_paths;
    m_allResultPaths.clear();
    m_resultQueue.clear();
    fetchMore();
}

bool CppIncludesIterator::hasNext() const
{
    return !m_resultQueue.isEmpty();
}

QString CppIncludesIterator::next()
{
    if (m_resultQueue.isEmpty())
        return QString();
    m_currentPath = m_resultQueue.takeFirst();
    if (m_resultQueue.isEmpty())
        fetchMore();
    return m_currentPath;
}

QString CppIncludesIterator::filePath() const
{
    return m_currentPath;
}

QString CppIncludesIterator::fileName() const
{
    return QFileInfo(m_currentPath).fileName();
}

void CppIncludesIterator::fetchMore()
{
    while (!m_queuedPaths.isEmpty() && m_resultQueue.isEmpty()) {
        const QString filePath = *m_queuedPaths.begin();
        m_queuedPaths.remove(filePath);
        CPlusPlus::Document::Ptr doc = m_snapshot.document(filePath);
        if (!doc)
            continue;
        foreach (const QString &includedPath, doc->includedFiles()) {
            if (!m_allResultPaths.contains(includedPath)) {
                m_allResultPaths.insert(includedPath);
                m_queuedPaths.insert(includedPath);
                m_resultQueue.append(includedPath);
            }
        }
    }
}

CppIncludesFilter::CppIncludesFilter()
    : m_needsUpdate(true)
{
    setId("All Included C/C++ Files");
    setDisplayName(tr("All Included C/C++ Files"));
    setShortcutString(QString(QLatin1Char('a')));
    setIncludedByDefault(true);
    setPriority(ILocatorFilter::Low);

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::fileListChanged,
            this, &CppIncludesFilter::markOutdated);
    connect(CppModelManager::instance(), &CppModelManager::documentUpdated,
            this, &CppIncludesFilter::markOutdated);
    connect(CppModelManager::instance(), &CppModelManager::aboutToRemoveFiles,
            this, &CppIncludesFilter::markOutdated);
    connect(DocumentModel::model(), &QAbstractItemModel::rowsInserted,
            this, &CppIncludesFilter::markOutdated);
    connect(DocumentModel::model(), &QAbstractItemModel::rowsRemoved,
            this, &CppIncludesFilter::markOutdated);
    connect(DocumentModel::model(), &QAbstractItemModel::dataChanged,
            this, &CppIncludesFilter::markOutdated);
    connect(DocumentModel::model(), &QAbstractItemModel::modelReset,
            this, &CppIncludesFilter::markOutdated);
}

void CppIncludesFilter::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)
    if (m_needsUpdate) {
        m_needsUpdate = false;
        QSet<QString> seedPaths;
        foreach (Project *project, SessionManager::projects()) {
            foreach (const QString &filePath, project->files(Project::AllFiles))
                seedPaths.insert(filePath);
        }
        foreach (DocumentModel::Entry *entry, DocumentModel::entries()) {
            if (entry)
                seedPaths.insert(entry->fileName().toString());
        }
        CPlusPlus::Snapshot snapshot = CppModelManager::instance()->snapshot();
        setFileIterator(new CppIncludesIterator(snapshot, seedPaths));
    }
    BaseFileFilter::prepareSearch(entry);
}

void CppIncludesFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    QTimer::singleShot(0, this, SLOT(markOutdated()));
}

void CppIncludesFilter::markOutdated()
{
    m_needsUpdate = true;
    setFileIterator(0); // clean up
}
