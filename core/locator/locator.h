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

#ifndef LOCATORPLUGIN_H
#define LOCATORPLUGIN_H

#include "ilocatorfilter.h"
#include "directoryfilter.h"
#include "executefilter.h"
#include "locatorconstants.h"

#include <core/actionmanager/command.h>

#include <QFutureWatcher>
#include <QObject>
#include <QTimer>

namespace Core {
namespace Internal {

class CorePlugin;
class LocatorWidget;
class OpenDocumentsFilter;
class FileSystemFilter;
class LocatorSettingsPage;
class ExternalToolsFilter;

class Locator : public QObject
{
    Q_OBJECT

public:
    Locator();
    ~Locator();

    void initialize(CorePlugin *corePlugin, const QStringList &arguments, QString *errorMessage);
    void extensionsInitialized();
    bool delayedInitialize();

    QList<ILocatorFilter *> filters();
    QList<ILocatorFilter *> customFilters();
    void setFilters(QList<ILocatorFilter *> f);
    void setCustomFilters(QList<ILocatorFilter *> f);
    int refreshInterval();
    void setRefreshInterval(int interval);

public slots:
    void refresh(QList<ILocatorFilter *> filters = QList<ILocatorFilter *>());
    void saveSettings();
    void openLocator();

private slots:
    void updatePlaceholderText(Core::Command *command = 0);

private:
    void loadSettings();
    void updateEditorManagerPlaceholderText();

    template <typename S>
    void loadSettingsHelper(S *settings);

    LocatorWidget *m_locatorWidget;
    LocatorSettingsPage *m_settingsPage;

    bool m_settingsInitialized;
    QList<ILocatorFilter *> m_filters;
    QList<ILocatorFilter *> m_customFilters;
    int m_refreshInterval;
    QTimer m_refreshTimer;
    OpenDocumentsFilter *m_openDocumentsFilter;
    FileSystemFilter *m_fileSystemFilter;
    ExecuteFilter *m_executeFilter;
    CorePlugin *m_corePlugin;
    ExternalToolsFilter *m_externalToolsFilter;
};

template <typename S>
void Locator::loadSettingsHelper(S *settings)
{
    settings->beginGroup(QLatin1String("QuickOpen"));
    m_refreshTimer.setInterval(settings->value(QLatin1String("RefreshInterval"), 60).toInt() * 60000);

    foreach (ILocatorFilter *filter, m_filters) {
        if (settings->contains(filter->id().toString())) {
            const QByteArray state = settings->value(filter->id().toString()).toByteArray();
            if (!state.isEmpty())
                filter->restoreState(state);
        }
    }
    settings->beginGroup(QLatin1String("CustomFilters"));
    QList<ILocatorFilter *> customFilters;
    const QStringList keys = settings->childKeys();
    int count = 0;
    Id baseId(Constants::CUSTOM_FILTER_BASEID);
    foreach (const QString &key, keys) {
        ILocatorFilter *filter = new DirectoryFilter(baseId.withSuffix(++count));
        filter->restoreState(settings->value(key).toByteArray());
        m_filters.append(filter);
        customFilters.append(filter);
    }
    setCustomFilters(customFilters);
    settings->endGroup();
    settings->endGroup();
}

} // namespace Internal
} // namespace Core

#endif // LOCATORPLUGIN_H
