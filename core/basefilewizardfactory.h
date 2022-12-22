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

#ifndef BASEFILEWIZARDFACTORY_H
#define BASEFILEWIZARDFACTORY_H

#include "core_global.h"
#include "generatedfile.h"

#include <core/iwizardfactory.h>

#include <QList>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QIcon;
class QWizard;
class QWizardPage;
class QDebug;
QT_END_NAMESPACE

namespace Utils { class Wizard; }

namespace Core {

class BaseFileWizard;

class CORE_EXPORT WizardDialogParameters
{
public:
    typedef QList<QWizardPage *> WizardPageList;

    enum DialogParameterEnum {
        ForceCapitalLetterForFileName = 0x01
    };
    Q_DECLARE_FLAGS(DialogParameterFlags, DialogParameterEnum)

    explicit WizardDialogParameters(const QString &defaultPath, const QString &platform,
                                    const FeatureSet &requiredFeatures, DialogParameterFlags flags,
                                    QVariantMap extraValues)
        : m_defaultPath(defaultPath),
          m_selectedPlatform(platform),
          m_requiredFeatures(requiredFeatures),
          m_parameterFlags(flags),
          m_extraValues(extraValues)
    {}

    QString defaultPath() const
    { return m_defaultPath; }

    QString selectedPlatform() const
    { return m_selectedPlatform; }

    FeatureSet requiredFeatures() const
    { return m_requiredFeatures; }

    DialogParameterFlags flags() const
    { return m_parameterFlags; }

    QVariantMap extraValues() const
    { return m_extraValues; }

private:
    QString m_defaultPath;
    QString m_selectedPlatform;
    FeatureSet m_requiredFeatures;
    DialogParameterFlags m_parameterFlags;
    QVariantMap m_extraValues;
};

class CORE_EXPORT BaseFileWizardFactory : public IWizardFactory
{
    Q_OBJECT

    friend class BaseFileWizard;

public:
    static QString buildFileName(const QString &path, const QString &baseName, const QString &extension);

protected:
    virtual BaseFileWizard *create(QWidget *parent, const WizardDialogParameters &parameters) const = 0;

    virtual GeneratedFiles generateFiles(const QWizard *w,
                                         QString *errorMessage) const = 0;

    virtual bool writeFiles(const GeneratedFiles &files, QString *errorMessage) const;

    virtual bool postGenerateFiles(const QWizard *w, const GeneratedFiles &l, QString *errorMessage) const;

    static QString preferredSuffix(const QString &mimeType);

    enum OverwriteResult { OverwriteOk,  OverwriteError,  OverwriteCanceled };
    OverwriteResult promptOverwrite(GeneratedFiles *files,
                                    QString *errorMessage) const;
    static bool postGenerateOpenEditors(const GeneratedFiles &l, QString *errorMessage = 0);

private:
    // IWizard
    Utils::Wizard *runWizardImpl(const QString &path, QWidget *parent, const QString &platform,
                                 const QVariantMap &extraValues) override;
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::GeneratedFile::Attributes)
Q_DECLARE_OPERATORS_FOR_FLAGS(Core::WizardDialogParameters::DialogParameterFlags)

#endif // BASEFILEWIZARDFACTORY_H
