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

#include "templateengine.h"

#include "qtcassert.h"

#ifndef QT_NO_JSENGINE
#include <QJSEngine>
#endif
#include <QStack>

namespace Utils {

namespace Internal {

// Preprocessor: Conditional section type.
enum PreprocessorSection {
    IfSection,
    ElsifSection,
    ElseSection,
    EndifSection,
    OtherSection
};

// Preprocessor: Section stack entry containing nested '@if' section
// state.
class PreprocessStackEntry {
public:
    PreprocessStackEntry(PreprocessorSection section = OtherSection,
                         bool parentEnabled = true,
                         bool condition = false,
                         bool anyIfClauseMatched = false);

    PreprocessorSection section;
    bool parentEnabled;
    bool condition;          // Current 'if/elsif' section is enabled.
    bool anyIfClauseMatched; // Determines if 'else' triggers
};

PreprocessStackEntry::PreprocessStackEntry(PreprocessorSection s, bool p, bool c, bool a) :
    section(s), parentEnabled(p), condition(c), anyIfClauseMatched(a)
{ }

// Context for preprocessing.
class PreprocessContext {
public:
    PreprocessContext();
    bool process(const QString &in, QString *out, QString *errorMessage);

private:
    void reset();
    PreprocessorSection preprocessorLine(const QString & in, QString *ifExpression) const;

    mutable QRegExp m_ifPattern;
    mutable QRegExp m_elsifPattern;
    mutable QRegExp m_elsePattern;
    mutable QRegExp m_endifPattern;

    QStack<PreprocessStackEntry> m_sectionStack;
#ifndef QT_NO_JSENGINE
    QJSEngine m_scriptEngine;
#endif
};

PreprocessContext::PreprocessContext() :
    // Cut out expression for 'if/elsif '
    m_ifPattern(QLatin1String("^([\\s]*@[\\s]*if[\\s]*)(.*)$")),
    m_elsifPattern(QLatin1String("^([\\s]*@[\\s]*elsif[\\s]*)(.*)$")),
    m_elsePattern(QLatin1String("^[\\s]*@[\\s]*else.*$")),
    m_endifPattern(QLatin1String("^[\\s]*@[\\s]*endif.*$"))
{
    QTC_CHECK(m_ifPattern.isValid() && m_elsifPattern.isValid()
              && m_elsePattern.isValid() && m_endifPattern.isValid());
}

void PreprocessContext::reset()
{
    m_sectionStack.clear();
    // Add a default, enabled section.
    m_sectionStack.push(PreprocessStackEntry(OtherSection, true, true));
}

// Determine type of line and return enumeration, cut out
// expression for '@if/@elsif'.
PreprocessorSection PreprocessContext::preprocessorLine(const QString &in,
                                                        QString *ifExpression) const
{
    if (m_ifPattern.exactMatch(in)) {
        *ifExpression = m_ifPattern.cap(2).trimmed();
        return IfSection;
    }
    if (m_elsifPattern.exactMatch(in)) {
        *ifExpression = m_elsifPattern.cap(2).trimmed();
        return ElsifSection;
    }

    ifExpression->clear();

    if (m_elsePattern.exactMatch(in))
        return ElseSection;
    if (m_endifPattern.exactMatch(in))
        return EndifSection;
    return OtherSection;
}

static inline QString msgEmptyStack(int line)
{
    return QString::fromLatin1("Unmatched '@endif' at line %1.").arg(line);
}

bool PreprocessContext::process(const QString &in, QString *out, QString *errorMessage)
{
    out->clear();
    if (in.isEmpty())
        return true;

    out->reserve(in.size());
    reset();

#ifdef QT_NO_JSENGINE
	if( errorMessage )
		*errorMessage = QString("Error in %1: no JSEngine").arg(__FILE__);
	return false;
#endif

    const QChar newLine = QLatin1Char('\n');
    const QStringList lines = in.split(newLine, QString::KeepEmptyParts);
    const int lineCount = lines.size();
    for (int l = 0; l < lineCount; l++) {
        // Check for element of the stack (be it dummy, else something is wrong).
        if (m_sectionStack.isEmpty()) {
            if (errorMessage)
                *errorMessage = msgEmptyStack(l);
            return false;
        }
        QString expression;
        bool expressionValue = false;
        PreprocessStackEntry &top = m_sectionStack.back();

        switch (preprocessorLine(lines.at(l), &expression)) {
        case IfSection:
            // '@If': Push new section
            if (top.condition) {
#ifndef QT_NO_JSENGINE
                if (!TemplateEngine::evaluateBooleanJavaScriptExpression(m_scriptEngine, expression,
                                                                         &expressionValue, errorMessage)) {
                    if (errorMessage)
                        *errorMessage = QString::fromLatin1("Error in @if at %1: %2")
                            .arg(l + 1).arg(*errorMessage);
                    return false;
                }
#endif
            }
            m_sectionStack.push(PreprocessStackEntry(IfSection,
                                                     top.condition, expressionValue, expressionValue));
            break;
        case ElsifSection: // '@elsif': Check condition.
            if (top.section != IfSection && top.section != ElsifSection) {
                if (errorMessage)
                    *errorMessage = QString::fromLatin1("No preceding @if found for @elsif at %1")
                        .arg(l + 1);
                return false;
            }
            if (top.parentEnabled) {
#ifndef QT_NO_JSENGINE
                if (!TemplateEngine::evaluateBooleanJavaScriptExpression(m_scriptEngine, expression,
                                                                         &expressionValue, errorMessage)) {
                    if (errorMessage)
                        *errorMessage = QString::fromLatin1("Error in @elsif at %1: %2")
                            .arg(l + 1).arg(*errorMessage);
                    return false;
                }
#endif
            }
            top.section = ElsifSection;
            // ignore consecutive '@elsifs' once something matched
            if (top.anyIfClauseMatched) {
                top.condition = false;
            } else {
                if ( (top.condition = expressionValue) )
                    top.anyIfClauseMatched = true;
            }
            break;
        case ElseSection: // '@else': Check condition.
            if (top.section != IfSection && top.section != ElsifSection) {
                if (errorMessage)
                    *errorMessage = QString::fromLatin1("No preceding @if/@elsif found for @else at %1")
                        .arg(l + 1);
                return false;
            }
            expressionValue = top.parentEnabled && !top.anyIfClauseMatched;
            top.section = ElseSection;
            top.condition = expressionValue;
            break;
        case EndifSection: // '@endif': Discard section.
            m_sectionStack.pop();
            break;
        case OtherSection: // Rest: Append according to current condition.
            if (top.condition) {
                if (l != 0)
                    out->append(newLine);
                out->append(lines.at(l));
            }
            break;
        } // switch section

    } // for lines
    return true;
}

} // namespace Internal

bool TemplateEngine::preprocessText(const QString &in, QString *out, QString *errorMessage)
{
    Internal::PreprocessContext context;
    return context.process(in, out, errorMessage);
}

QString TemplateEngine::processText(MacroExpander *expander, const QString &input,
                                    QString *errorMessage)
{
    if (errorMessage)
        errorMessage->clear();

    if (input.isEmpty())
        return input;

    // Recursively expand macros:
    QString in = input;
    QString oldIn;
    for (int i = 0; i < 5 && in != oldIn; ++i) {
        oldIn = in;
        in = expander->expand(oldIn);
    }

    QString out;
    if (!preprocessText(in, &out, errorMessage))
        return QString();

    // Expand \n, \t and handle line continuation:
    QString result;
    result.reserve(out.count());
    bool isEscaped = false;
    for (int i = 0; i < out.count(); ++i) {
        const QChar c = out.at(i);

        if (isEscaped) {
            if (c == QLatin1Char('n'))
                result.append(QLatin1Char('\n'));
            else if (c == QLatin1Char('t'))
                result.append(QLatin1Char('\t'));
            else if (c != QLatin1Char('\n'))
                result.append(c);
            isEscaped = false;
        } else {
            if (c == QLatin1Char('\\'))
                isEscaped = true;
            else
                result.append(c);
        }
    }
    return result;
}

bool TemplateEngine::evaluateBooleanJavaScriptExpression(QJSEngine &engine,
                                                         const QString &expression, bool *result,
                                                         QString *errorMessage)
{
    if (errorMessage)
        errorMessage->clear();
    if (result)
        *result = false;       
#ifndef QT_NO_JSENGINE
    const QJSValue value = engine.evaluate(expression);
    if (value.isError()) {
        if (errorMessage)
            *errorMessage = QString::fromLatin1("Error in \"%1\": %2")
                .arg(expression, value.toString());
        return false;
    }
    // Try to convert to bool, be that an int or whatever.
    if (value.isBool()) {
        if (result)
            *result = value.toBool();
        return true;
    }
    if (value.isNumber()) {
        if (result)
            *result = !qFuzzyCompare(value.toNumber(), 0);
        return true;
    }
    if (value.isString()) {
        if (result)
            *result = !value.toString().isEmpty();
        return true;
    }
    if (errorMessage)
        *errorMessage = QString::fromLatin1("Cannot convert result of \"%1\" (\"%2\"to bool.")
            .arg(expression, value.toString());
#endif
    return false;
}

} // namespace Utils
