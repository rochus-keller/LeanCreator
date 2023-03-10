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


#include "generatedfile.h"

#include <utils/fileutils.h>

#include <QString>
#include <QDir>
#include <QCoreApplication>

namespace Core {

/*!
    \class Core::GeneratedFile
    \brief The GeneratedFile class represents a file generated by a wizard.

    The Wizard class checks whether each file already exists and
    reports any errors that may occur during creation of the files.

    \sa Core::BaseFileWizardParameters, Core::BaseFileWizard, Core::StandardFileWizard
    \sa Core::Internal::WizardEventLoop
 */



class GeneratedFilePrivate : public QSharedData
{
public:
    GeneratedFilePrivate() : binary(false) {}
    explicit GeneratedFilePrivate(const QString &p);
    QString path;
    QByteArray contents;
    Id editorId;
    bool binary;
    GeneratedFile::Attributes attributes;
};

GeneratedFilePrivate::GeneratedFilePrivate(const QString &p) :
    path(QDir::cleanPath(p)),
    binary(false),
    attributes(0)
{
}

GeneratedFile::GeneratedFile() :
    m_d(new GeneratedFilePrivate)
{
}

GeneratedFile::GeneratedFile(const QString &p) :
    m_d(new GeneratedFilePrivate(p))
{
}

GeneratedFile::GeneratedFile(const GeneratedFile &rhs) :
    m_d(rhs.m_d)
{
}

GeneratedFile &GeneratedFile::operator=(const GeneratedFile &rhs)
{
    if (this != &rhs)
        m_d.operator=(rhs.m_d);
    return *this;
}

GeneratedFile::~GeneratedFile()
{
}

QString GeneratedFile::path() const
{
    return m_d->path;
}

void GeneratedFile::setPath(const QString &p)
{
    m_d->path = QDir::cleanPath(p);
}

QString GeneratedFile::contents() const
{
    return QString::fromUtf8(m_d->contents);
}

void GeneratedFile::setContents(const QString &c)
{
    m_d->contents = c.toUtf8();
}

QByteArray GeneratedFile::binaryContents() const
{
    return m_d->contents;
}

void GeneratedFile::setBinaryContents(const QByteArray &c)
{
    m_d->contents = c;
}

bool GeneratedFile::isBinary() const
{
    return m_d->binary;
}

void GeneratedFile::setBinary(bool b)
{
    m_d->binary = b;
}

Id GeneratedFile::editorId() const
{
    return m_d->editorId;
}

void GeneratedFile::setEditorId(Id id)
{
    m_d->editorId = id;
}

bool GeneratedFile::write(QString *errorMessage) const
{
    // Ensure the directory
    const QFileInfo info(m_d->path);
    const QDir dir = info.absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(dir.absolutePath())) {
            *errorMessage = QCoreApplication::translate("BaseFileWizard", "Unable to create the directory %1.").arg(
                        QDir::toNativeSeparators(dir.absolutePath()));
            return false;
        }
    }

    // Write out
    QIODevice::OpenMode flags = QIODevice::WriteOnly|QIODevice::Truncate;
    if (!isBinary())
        flags |= QIODevice::Text;

    Utils::FileSaver saver(m_d->path, flags);
    saver.write(m_d->contents);
    return saver.finalize(errorMessage);
}

GeneratedFile::Attributes GeneratedFile::attributes() const
{
    return m_d->attributes;
}

void GeneratedFile::setAttributes(Attributes a)
{
    m_d->attributes = a;
}

} // namespace Core
