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

#ifndef FILEICONPROVIDER_H
#define FILEICONPROVIDER_H

#include <core/core_global.h>

#include <QStyle>
#include <QFileIconProvider>

namespace Core {

namespace FileIconProvider {

// Access to the single instance
CORE_EXPORT QFileIconProvider *iconProvider();

// Access to individual items
CORE_EXPORT QIcon icon(const QFileInfo &info);
CORE_EXPORT QIcon icon(QFileIconProvider::IconType type);

// Register additional overlay icons
CORE_EXPORT QPixmap overlayIcon(QStyle::StandardPixmap baseIcon, const QIcon &overlayIcon, const QSize &size);
CORE_EXPORT void registerIconOverlayForSuffix(const char *path, const char *suffix);
CORE_EXPORT void registerIconOverlayForMimeType(const char *path, const char *mimeType);
CORE_EXPORT void registerIconOverlayForMimeType(const QIcon &icon, const char *mimeType);

} // namespace FileIconProvider
} // namespace Core

#endif // FILEICONPROVIDER_H
