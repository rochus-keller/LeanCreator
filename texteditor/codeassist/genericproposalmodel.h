/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of LeanCreator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef IGENERICPROPOSALMODEL_H
#define IGENERICPROPOSALMODEL_H

#include "iassistproposalmodel.h"
#include "assistenums.h"

#include <texteditor/texteditor_global.h>


#include <QHash>
#include <QList>

QT_FORWARD_DECLARE_CLASS(QIcon)

namespace TextEditor {

class AssistProposalItem;

class TEXTEDITOR_EXPORT GenericProposalModel : public IAssistProposalModel
{
public:
    GenericProposalModel();
    ~GenericProposalModel();

    void reset() override;
    int size() const override;
    QString text(int index) const override;

    virtual QIcon icon(int index) const;
    virtual QString detail(int index) const;
    virtual int persistentId(int index) const;
    virtual void removeDuplicates();
    virtual void filter(const QString &prefix);
    virtual bool isSortable(const QString &prefix) const;
    virtual void sort(const QString &prefix);
    virtual bool supportsPrefixExpansion() const;
    virtual QString proposalPrefix() const;
    virtual bool keepPerfectMatch(AssistReason reason) const;
    virtual AssistProposalItem *proposalItem(int index) const;

    void loadContent(const QList<AssistProposalItem *> &items);
    void setSortingAllowed(bool isAllowed);
    bool isSortingAllowed() const;

    Qt::TextFormat detailTextFormat() const;
    void setDetailTextFormat(Qt::TextFormat detailTextFormat);

protected:
    QList<AssistProposalItem *> m_currentItems;

private:
    QHash<QString, int> m_idByText;
    QList<AssistProposalItem *> m_originalItems;
    Qt::TextFormat m_detailTextFormat;
};
} // TextEditor

#endif // IGENERICPROPOSALMODEL_H
