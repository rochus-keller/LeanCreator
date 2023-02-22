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

#include "versiondialog.h"

#include <app/app_version.h>
#include <core/coreconstants.h>
#include <core/icore.h>
#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>

using namespace Core;
using namespace Core::Internal;

VersionDialog::VersionDialog(QWidget *parent)
    : QDialog(parent)
{
    // We need to set the window icon explicitly here since for some reason the
    // application icon isn't used when the size of the dialog is fixed (at least not on X11/GNOME)
    if (Utils::HostOsInfo::isLinuxHost())
        setWindowIcon(QIcon(QLatin1String(Constants::ICON_QTLOGO_128)));

    setWindowTitle(tr("About LeanCreator"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QGridLayout *layout = new QGridLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);

    QString ideRev;
#ifdef IDE_REVISION
     //: This gets conditionally inserted as argument %8 into the description string.
     ideRev = tr("<br/>From revision %1<br/>").arg(QString::fromLatin1(Constants::IDE_REVISION_STR).left(10));
#endif
     QString buildDateInfo;
#ifdef QTC_SHOW_BUILD_DATE
     buildDateInfo = tr("<br/>Built on %1 %2<br/>").arg(QLatin1String(__DATE__), QLatin1String(__TIME__));
#endif

    const QString br = QLatin1String("<br/>");
    const QStringList additionalInfoLines = ICore::additionalAboutInformation();
    const QString additionalInfo =
            QStringList(Utils::transform(additionalInfoLines, &QString::toHtmlEscaped)).join(br);

    const QString description = tr(
        "<h3>%1</h3>"
        "%2<br/>"
        "%3"
        "%4"
        "%5"
        "<br/>"
        "<p>See <a href=\"https://github.com/rochus-keller/LeanCreator\">the Github page</a> "
                "for more information.</p>"
        "<p>LeanCreator is available under GNU GPL/LGPL version 2.1 or version 3.</p>" )
        .arg(ICore::versionString(),
             ICore::buildCompatibilityString(),
             buildDateInfo,
             ideRev,
             additionalInfo.isEmpty() ? QString() : br + additionalInfo + br);

    QLabel *copyRightLabel = new QLabel(description);
    copyRightLabel->setWordWrap(true);
    copyRightLabel->setOpenExternalLinks(true);
    copyRightLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    QPushButton *closeButton = buttonBox->button(QDialogButtonBox::Close);
    QTC_CHECK(closeButton);
    buttonBox->addButton(closeButton, QDialogButtonBox::ButtonRole(QDialogButtonBox::RejectRole | QDialogButtonBox::AcceptRole));
    connect(buttonBox , SIGNAL(rejected()), this, SLOT(reject()));

    QLabel *logoLabel = new QLabel;
    logoLabel->setPixmap(QPixmap(QLatin1String(Constants::ICON_QTLOGO_128)));
    layout->addWidget(logoLabel , 0, 0, 1, 1);
    layout->addWidget(copyRightLabel, 0, 1, 4, 4);
    layout->addWidget(buttonBox, 4, 0, 1, 5);
}

bool VersionDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
            ke->accept();
            return true;
        }
    }
    return QDialog::event(event);
}
