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

#ifndef CUSTOMTOOLCHAIN_H
#define CUSTOMTOOLCHAIN_H

#include "projectexplorer_export.h"

#include "abi.h"
#include "customparser.h"
#include "headerpath.h"
#include "toolchain.h"
#include "toolchainconfigwidget.h"

#include <utils/fileutils.h>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QTextEdit;
class QComboBox;
class QPushButton;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace ProjectExplorer {

class AbiWidget;

namespace Internal { class CustomToolChainFactory; }
// --------------------------------------------------------------------------
// CustomToolChain
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT CustomToolChain : public ToolChain
{
    Q_DECLARE_TR_FUNCTIONS(CustomToolChain)

public:
    enum OutputParser
    {
        Gcc = 0,
        Clang = 1,
        LinuxIcc = 2,
#if defined(Q_OS_WIN)
        Msvc = 3,
#endif
        Custom,
        OutputParserCount
    };

    QString typeDisplayName() const override;
    Abi targetAbi() const override;
    void setTargetAbi(const Abi &);

    bool isValid() const override;

    QByteArray predefinedMacros(const QStringList &cxxflags) const override;
    CompilerFlags compilerFlags(const QStringList &cxxflags) const override;
    WarningFlags warningFlags(const QStringList &cxxflags) const override;
    const QStringList &rawPredefinedMacros() const;
    void setPredefinedMacros(const QStringList &list);

    QList<HeaderPath> systemHeaderPaths(const QStringList &cxxFlags,
                                        const Utils::FileName &) const override;
    void addToEnvironment(Utils::Environment &env) const override;
    QList<Utils::FileName> suggestedMkspecList() const override;
    IOutputParser *outputParser() const override;
    QStringList headerPathsList() const;
    void setHeaderPaths(const QStringList &list);

    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;

    ToolChainConfigWidget *configurationWidget() override;

    bool operator ==(const ToolChain &) const override;

    void setCompilerCommand(const Utils::FileName &);
    Utils::FileName compilerCommand() const override;
    void setMakeCommand(const Utils::FileName &);
    QString makeCommand(const Utils::Environment &environment) const override;

    void setCxx11Flags(const QStringList &);
    const QStringList &cxx11Flags() const;

    void setMkspecs(const QString &);
    QString mkspecs() const;

    ToolChain *clone() const override;

    OutputParser outputParserType() const;
    void setOutputParserType(OutputParser parser);
    CustomParserSettings customParserSettings() const;
    void setCustomParserSettings(const CustomParserSettings &settings);
    static QString parserName(OutputParser parser);

protected:
    CustomToolChain(const CustomToolChain &) = default;

private:
    explicit CustomToolChain(Detection d);

    Utils::FileName m_compilerCommand;
    Utils::FileName m_makeCommand;

    Abi m_targetAbi;
    QStringList m_predefinedMacros;
    QList<HeaderPath> m_systemHeaderPaths;
    QStringList m_cxx11Flags;
    QList<Utils::FileName> m_mkspecs;

    OutputParser m_outputParser;
    CustomParserSettings m_customParserSettings;

    friend class Internal::CustomToolChainFactory;
    friend class ToolChainFactory;
};

namespace Internal {

class CustomToolChainFactory : public ToolChainFactory
{
    Q_OBJECT

public:
    CustomToolChainFactory();

    bool canCreate() override;
    ToolChain *create() override;

    // Used by the ToolChainManager to restore user-generated tool chains
    bool canRestore(const QVariantMap &data) override;
    ToolChain *restore(const QVariantMap &data) override;
};

// --------------------------------------------------------------------------
// CustomToolChainConfigWidget
// --------------------------------------------------------------------------

class TextEditDetailsWidget;

class CustomToolChainConfigWidget : public ToolChainConfigWidget
{
    Q_OBJECT

public:
    CustomToolChainConfigWidget(CustomToolChain *);

private slots:
    void updateSummaries();
    void errorParserChanged(int index);
    void openCustomParserSettingsDialog();

protected:
    void applyImpl() override;
    void discardImpl() override { setFromToolchain(); }
    bool isDirtyImpl() const override;
    void makeReadOnlyImpl() override;

    void setFromToolchain();

    Utils::PathChooser *m_compilerCommand;
    Utils::PathChooser *m_makeCommand;
    AbiWidget *m_abiWidget;
    QPlainTextEdit *m_predefinedMacros;
    QPlainTextEdit *m_headerPaths;
    TextEditDetailsWidget *m_predefinedDetails;
    TextEditDetailsWidget *m_headerDetails;
    QLineEdit *m_cxx11Flags;
    QLineEdit *m_mkspecs;
    QComboBox *m_errorParserComboBox;
    QPushButton *m_customParserSettingsButton;

    CustomParserSettings m_customParserSettings;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // CUSTOMTOOLCHAIN_H
