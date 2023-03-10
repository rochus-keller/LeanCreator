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

#ifndef PLUGINSPEC_P_H
#define PLUGINSPEC_P_H

#include "pluginspec.h"
#include "iplugin.h"

#include <QJsonObject>
#include <QObject>
#include <QPluginLoader>
#include <QRegExp>
#include <QStringList>
#include <QVector>
#include <QXmlStreamReader>

namespace ExtensionSystem {

class IPlugin;
class PluginManager;

namespace Internal {

class EXTENSIONSYSTEM_EXPORT PluginSpecPrivate : public QObject
{
    Q_OBJECT

public:
    PluginSpecPrivate(PluginSpec *spec);

    bool read(const QString &fileName);
    bool read(const QStaticPlugin&);
    bool provides(const QString &pluginName, const QString &version) const;
    bool resolveDependencies(const QList<PluginSpec *> &specs);
    bool loadLibrary();
    bool initializePlugin();
    bool initializeExtensions();
    bool delayedInitialize();
    IPlugin::ShutdownFlag stop();
    void kill();

    void setEnabledBySettings(bool value);
    void setEnabledByDefault(bool value);
    void setForceEnabled(bool value);
    void setForceDisabled(bool value);

#ifndef QT_NO_PLUGINS
    QPluginLoader loader;
#endif

    QString name;
    QString version;
    QString compatVersion;
    bool required;
    bool experimental;
    bool enabledByDefault;
    QString vendor;
    QString copyright;
    QString license;
    QString description;
    QString url;
    QString category;
    QRegExp platformSpecification;
    QVector<PluginDependency> dependencies;
    bool enabledBySettings;
    bool enabledIndirectly;
    bool forceEnabled;
    bool forceDisabled;

    QString location;
    QString filePath;
    QStringList arguments;

    QHash<PluginDependency, PluginSpec *> dependencySpecs;
    PluginSpec::PluginArgumentDescriptions argumentDescriptions;
    IPlugin *plugin;
    QObject* candidate;

    PluginSpec::State state;
    bool hasError;
    QString errorString;

    static bool isValidVersion(const QString &version);
    static int versionCompare(const QString &version1, const QString &version2);

    void enableDependenciesIndirectly();

    bool readMetaData(const QJsonObject &metaData);

private:
    PluginSpec *q;

    bool reportError(const QString &err);
    static QRegExp &versionRegExp();
};

} // namespace Internal
} // namespace ExtensionSystem

#endif // PLUGINSPEC_P_H
