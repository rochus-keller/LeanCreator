/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef IOSXCODEPARSER_H
#define IOSXCODEPARSER_H

#include "projectexplorer_export.h"
#include "ioutputparser.h"
#ifndef QT_NO_DEVICESUPPORT
#include "devicesupport/idevice.h"
#endif

#include <QRegExp>
#include <QStringList>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT XcodebuildParser : public IOutputParser
{
    Q_OBJECT
public:
    enum XcodebuildStatus {
        InXcodebuild,
        OutsideXcodebuild,
        UnknownXcodebuildState
    };

    XcodebuildParser();

    void stdOutput(const QString &line);
    void stdError(const QString &line);
    bool hasFatalErrors() const;
private:
    int m_fatalErrorCount;
    QRegExp m_failureRe;
    QRegExp m_successRe;
    QRegExp m_buildRe;
    QRegExp m_replacingSignatureRe;
    XcodebuildStatus m_xcodeBuildParserState;
    QString m_lastTarget;
    QString m_lastProject;
#if defined WITH_TESTS
    friend class XcodebuildParserTester;
    friend class ProjectExplorerPlugin;
#endif
};

#if defined WITH_TESTS
class XcodebuildParserTester : public QObject
{
    Q_OBJECT
public:
    explicit XcodebuildParserTester(XcodebuildParser *parser, QObject *parent = 0);

    XcodebuildParser *parser;
    XcodebuildParser::XcodebuildStatus expectedFinalState;

public slots:
    void onAboutToDeleteParser();
};
#endif

} // namespace ProjectExplorer

#endif // IOSXCODEPARSER_H
