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

#include "fadingindicator.h"

#include "stylehelper.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#ifndef QT_NO_ANIMATION
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#endif
#include <QTimer>

namespace Utils {
namespace Internal {

class FadingIndicatorPrivate : public QWidget
{
#ifndef QT_NO_ANIMATION
    Q_OBJECT
#endif
public:
    FadingIndicatorPrivate(QWidget *parent, FadingIndicator::TextSize size)
        : QWidget(parent)
    {
#ifndef QT_NO_ANIMATION
        m_effect = new QGraphicsOpacityEffect(this);
        setGraphicsEffect(m_effect);
        m_effect->setOpacity(1.);
#endif

        m_label = new QLabel;
        QFont font = m_label->font();
        font.setPixelSize(size == FadingIndicator::LargeText ? 45 : 22);
        m_label->setFont(font);
        QPalette pal = palette();
        pal.setColor(QPalette::Foreground, pal.color(QPalette::Background));
        m_label->setPalette(pal);
        auto layout = new QHBoxLayout;
        setLayout(layout);
        layout->addWidget(m_label);
    }

    void setText(const QString &text)
    {
        m_pixmap = QPixmap();
        m_label->setText(text);
        layout()->setSizeConstraint(QLayout::SetFixedSize);
        adjustSize();
        if (QWidget *parent = parentWidget())
            move(parent->rect().center() - rect().center());
    }

    void setPixmap(const QString &uri)
    {
        m_label->hide();
        m_pixmap.load(Utils::StyleHelper::dpiSpecificImageFile(uri));
        layout()->setSizeConstraint(QLayout::SetNoConstraint);
        resize(m_pixmap.size() / m_pixmap.devicePixelRatio());
        if (QWidget *parent = parentWidget())
            move(parent->rect().center() - rect().center());
    }

    void run(int ms)
    {
        show();
        raise();
#ifndef QT_NO_ANIMATION
        QTimer::singleShot(ms, this, SLOT(runInternal()));
#endif
    }

protected:
    void paintEvent(QPaintEvent *)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        if (!m_pixmap.isNull()) {
            p.drawPixmap(rect(), m_pixmap);
        } else {
            p.setBrush(palette().color(QPalette::Foreground));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(rect(), 15, 15);
        }
    }

private slots:
    void runInternal()
    {
#ifndef QT_NO_ANIMATION
        QPropertyAnimation *anim = new QPropertyAnimation(m_effect, "opacity", this);
        anim->setDuration(200);
        anim->setEndValue(0.);
        connect(anim, &QAbstractAnimation::finished, this, &QObject::deleteLater);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
#endif
    }

private:
#ifndef QT_NO_ANIMATION
    QGraphicsOpacityEffect *m_effect;
#endif
    QLabel *m_label;
    QPixmap m_pixmap;
};

} // Internal

namespace FadingIndicator {

void showText(QWidget *parent, const QString &text, TextSize size)
{
    static QPointer<Internal::FadingIndicatorPrivate> indicator;
    if (indicator)
        delete indicator;
    indicator = new Internal::FadingIndicatorPrivate(parent, size);
    indicator->setText(text);
    indicator->run(2500); // deletes itself
}

void showPixmap(QWidget *parent, const QString &pixmap)
{
    static QPointer<Internal::FadingIndicatorPrivate> indicator;
    if (indicator)
        delete indicator;
    indicator = new Internal::FadingIndicatorPrivate(parent, LargeText);
    indicator->setPixmap(pixmap);
    indicator->run(300); // deletes itself
}

} // FadingIndicator
} // Utils

//#include "fadingindicator.moc"
