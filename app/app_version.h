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

#ifndef APP_VERSION_H
#define APP_VERSION_H

namespace Core {
namespace Constants {

#define STRINGIFY_INTERNAL(x) #x
#define STRINGIFY(x) STRINGIFY_INTERNAL(x)

#define IDE_VERSION 3.6.1
#define IDE_VERSION_STR STRINGIFY(IDE_VERSION)

#define IDE_VERSION_MAJOR 3
#define IDE_VERSION_MINOR 6
#define IDE_VERSION_RELEASE 1

const char * const IDE_VERSION_LONG      = IDE_VERSION_STR;
const char * const IDE_AUTHOR            = "The Qt Company Ltd";
const char * const IDE_YEAR              = "2016";

#ifdef IDE_VERSION_DESCRIPTION
const char * const IDE_VERSION_DESCRIPTION_STR = STRINGIFY(IDE_VERSION_DESCRIPTION);
#else
const char * const IDE_VERSION_DESCRIPTION_STR = "";
#endif

#ifdef IDE_REVISION
const char * const IDE_REVISION_STR      = STRINGIFY(IDE_REVISION);
#else
const char * const IDE_REVISION_STR      = "";
#endif

// changes the path where the settings are saved to
#ifdef IDE_SETTINGSVARIANT
const char * const IDE_SETTINGSVARIANT_STR      = STRINGIFY(IDE_SETTINGSVARIANT);
#else
const char * const IDE_SETTINGSVARIANT_STR      = "LeanQtProject";
#endif

#ifdef IDE_COPY_SETTINGS_FROM_VARIANT
const char * const IDE_COPY_SETTINGS_FROM_VARIANT_STR = STRINGIFY(IDE_COPY_SETTINGS_FROM_VARIANT);
#else
const char * const IDE_COPY_SETTINGS_FROM_VARIANT_STR = "Nokia";
#endif


#undef IDE_VERSION
#undef IDE_VERSION_STR
#undef STRINGIFY
#undef STRINGIFY_INTERNAL

} // Constants
} // Core

#endif //APP_VERSION_H
