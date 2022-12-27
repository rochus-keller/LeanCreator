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

#ifndef MIMETYPEMAGICDIALOG_H
#define MIMETYPEMAGICDIALOG_H

#include "ui_mimetypemagicdialog.h"

#include <utils/mimetypes/mimemagicrule_p.h>

namespace Core {
namespace Internal {

class MagicData
{
public:
    MagicData()
        : m_rule(Utils::Internal::MimeMagicRule::String, QByteArray(" "), 0, 0),
          m_priority(0)
    {
    }

    MagicData(Utils::Internal::MimeMagicRule rule, int priority)
        : m_rule(rule)
        , m_priority(priority)
    {
    }

    bool operator==(const MagicData &other);
    bool operator!=(const MagicData &other) { return !(*this == other); }

    static QByteArray normalizedMask(const Utils::Internal::MimeMagicRule &rule);

    Utils::Internal::MimeMagicRule m_rule;
    int m_priority;
};

class MimeTypeMagicDialog : public QDialog
{
    Q_DECLARE_TR_FUNCTIONS(Core::Internal::MimeTypeMagicDialog)
public:
    explicit MimeTypeMagicDialog(QWidget *parent = 0);

    void setMagicData(const MagicData &data);
    MagicData magicData() const;

private:
    void setToRecommendedValues();
    void applyRecommended(bool checked);
    void validateAccept();
    Utils::Internal::MimeMagicRule createRule(QString *errorMessage = 0) const;

    Ui::MimeTypeMagicDialog ui;
    int m_customRangeStart;
    int m_customRangeEnd;
    int m_customPriority;
};

} // Internal
} // Core

Q_DECLARE_METATYPE(Core::Internal::MagicData)

#endif // MIMETYPEMAGICDIALOG_H
