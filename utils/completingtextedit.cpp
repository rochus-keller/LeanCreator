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

#include "completingtextedit_p.h"

#include <QAbstractItemView>
#include <QKeyEvent>
#include <QScrollBar>

static bool isEndOfWordChar(const QChar &c)
{
    return !c.isLetterOrNumber() && c.category() != QChar::Punctuation_Connector;
}

/*! \class Utils::CompletingTextEdit

  \brief The CompletingTextEdit class is a QTextEdit with auto-completion
  support.

  Excerpted from Qt examples/tools/customcompleter.
*/

namespace Utils {

CompletingTextEditPrivate::CompletingTextEditPrivate(CompletingTextEdit *textEdit)
    : m_completer(0),
      m_completionLengthThreshold(3),
      m_backPointer(textEdit)
{
}

void CompletingTextEditPrivate::insertCompletion(const QString &completion)
{
    if (m_completer->widget() != m_backPointer)
        return;
    QTextCursor tc = m_backPointer->textCursor();
    int extra = completion.length() - m_completer->completionPrefix().length();
    tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::EndOfWord);
    tc.insertText(completion.right(extra));
    m_backPointer->setTextCursor(tc);
}

QString CompletingTextEditPrivate::textUnderCursor() const
{
    QTextCursor tc = m_backPointer->textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

bool CompletingTextEditPrivate::acceptsCompletionPrefix(const QString &prefix) const
{
    return prefix.length() >= m_completionLengthThreshold;
}

CompletingTextEdit::CompletingTextEdit(QWidget *parent)
    : QTextEdit(parent),
      d(new CompletingTextEditPrivate(this))
{
}

CompletingTextEdit::~CompletingTextEdit()
{
    delete d;
}

void CompletingTextEdit::setCompleter(QCompleter *c)
{
    if (completer())
        disconnect(completer(), 0, this, 0);

    d->m_completer = c;

    if (!c)
        return;

    completer()->setWidget(this);
    completer()->setCompletionMode(QCompleter::PopupCompletion);
    connect(completer(), static_cast<void (QCompleter::*)(const QString &)>(&QCompleter::activated),
            this, [this](const QString &str) { d->insertCompletion(str); });
}

QCompleter *CompletingTextEdit::completer() const
{
    return d->m_completer;
}

int CompletingTextEdit::completionLengthThreshold() const
{
    return d->m_completionLengthThreshold;
}

void CompletingTextEdit::setCompletionLengthThreshold(int len)
{
    d->m_completionLengthThreshold = len;
}

void CompletingTextEdit::keyPressEvent(QKeyEvent *e)
{
    if (completer() && completer()->popup()->isVisible()) {
        // The following keys are forwarded by the completer to the widget
        switch (e->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            e->ignore();
            return; // let the completer do default behavior
        default:
            break;
        }
    }

    const bool isShortcut = ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_E); // CTRL+E
    if (completer() == 0 || !isShortcut) // do not process the shortcut when we have a completer
        QTextEdit::keyPressEvent(e);

    const bool ctrlOrShift = e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
    const QString text = e->text();
    if (completer() == 0 || (ctrlOrShift && text.isEmpty()))
        return;

    const bool hasModifier = (e->modifiers() != Qt::NoModifier) && !ctrlOrShift;
    const QString newCompletionPrefix = d->textUnderCursor();
    const QChar lastChar = text.isEmpty() ? QChar() : text.right(1).at(0);

    if (!isShortcut && (hasModifier || text.isEmpty() || isEndOfWordChar(lastChar)
                        || !d->acceptsCompletionPrefix(newCompletionPrefix))) {
        completer()->popup()->hide();
        return;
    }

    if (newCompletionPrefix != completer()->completionPrefix()) {
        completer()->setCompletionPrefix(newCompletionPrefix);
        completer()->popup()->setCurrentIndex(completer()->completionModel()->index(0, 0));
    }
    QRect cr = cursorRect();
    cr.setWidth(completer()->popup()->sizeHintForColumn(0)
                + completer()->popup()->verticalScrollBar()->sizeHint().width());
    completer()->complete(cr); // popup it up!
}

void CompletingTextEdit::focusInEvent(QFocusEvent *e)
{
    if (completer() != 0)
        completer()->setWidget(this);
    QTextEdit::focusInEvent(e);
}

bool CompletingTextEdit::event(QEvent *e)
{
    // workaround for QTCREATORBUG-9453
    if (e->type() == QEvent::ShortcutOverride && completer()
            && completer()->popup() && completer()->popup()->isVisible()) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
            ke->accept();
            return true;
        }
    }
    return QTextEdit::event(e);
}

} // namespace Utils

//#include "moc_completingtextedit.cpp"
