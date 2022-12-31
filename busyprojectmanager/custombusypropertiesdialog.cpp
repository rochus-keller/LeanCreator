/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "custombusypropertiesdialog.h"
#include "ui_custombusypropertiesdialog.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QTableWidgetItem>

namespace BusyProjectManager {
namespace Internal {

CustomBusyPropertiesDialog::CustomBusyPropertiesDialog(const QVariantMap &properties, QWidget *parent)
    : QDialog(parent), m_ui(new Ui::CustomBusyPropertiesDialog)
{
    m_ui->setupUi(this);
    m_ui->propertiesTable->setRowCount(properties.count());
    m_ui->propertiesTable->setHorizontalHeaderLabels(QStringList() << tr("Key") << tr("Value"));
    int currentRow = 0;
    for (QVariantMap::ConstIterator it = properties.constBegin(); it != properties.constEnd();
         ++it) {
        QTableWidgetItem * const nameItem = new QTableWidgetItem;
        nameItem->setData(Qt::DisplayRole, it.key());
        m_ui->propertiesTable->setItem(currentRow, 0, nameItem);
        QTableWidgetItem * const valueItem = new QTableWidgetItem;
        valueItem->setData(Qt::DisplayRole, it.value());
        m_ui->propertiesTable->setItem(currentRow, 1, valueItem);
        ++currentRow;
    }
    connect(m_ui->addButton, SIGNAL(clicked()), SLOT(addProperty()));
    connect(m_ui->removeButton, SIGNAL(clicked()), SLOT(removeSelectedProperty()));
    connect(m_ui->propertiesTable, SIGNAL(currentItemChanged(QTableWidgetItem*,QTableWidgetItem*)),
            SLOT(handleCurrentItemChanged()));
    handleCurrentItemChanged();
}

QVariantMap CustomBusyPropertiesDialog::properties() const
{
    QVariantMap properties;
    for (int row = 0; row < m_ui->propertiesTable->rowCount(); ++row) {
        const QTableWidgetItem * const nameItem = m_ui->propertiesTable->item(row, 0);
        const QString name = nameItem->text();
        if (name.isEmpty())
            continue;
        properties.insert(name, m_ui->propertiesTable->item(row, 1)->text());
    }
    return properties;
}

CustomBusyPropertiesDialog::~CustomBusyPropertiesDialog()
{
    delete m_ui;
}

void CustomBusyPropertiesDialog::addProperty()
{
    const int row = m_ui->propertiesTable->rowCount();
    m_ui->propertiesTable->insertRow(row);
    m_ui->propertiesTable->setItem(row, 0, new QTableWidgetItem);
    m_ui->propertiesTable->setItem(row, 1, new QTableWidgetItem);
}

void CustomBusyPropertiesDialog::removeSelectedProperty()
{
    const QTableWidgetItem * const currentItem = m_ui->propertiesTable->currentItem();
    QTC_ASSERT(currentItem, return);
    m_ui->propertiesTable->removeRow(currentItem->row());
}

void CustomBusyPropertiesDialog::handleCurrentItemChanged()
{
    m_ui->removeButton->setEnabled(m_ui->propertiesTable->currentItem());
}

} // namespace Internal
} // namespace BusyProjectManager
