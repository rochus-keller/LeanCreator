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

#ifndef BUSYCONSTANTS_H
#define BUSYCONSTANTS_H

namespace BusyProjectManager {
namespace Constants {

// Toolchain related settings:
const char BUSY_TARGETOS[] = "busy.targetOS";
const char BUSY_SYSROOT[] = "busy.sysroot";
const char BUSY_ARCHITECTURE[] = "busy.architecture";
const char BUSY_TOOLCHAIN[] = "busy.toolchain";
const char CPP_TOOLCHAINPATH[] = "busy.toolchainInstallPath";
const char CPP_TOOLCHAINPREFIX[] = "busy.toolchainPrefix";
const char CPP_COMPILERNAME[] = "busy.compilerName";
const char CPP_CXXCOMPILERNAME[] = "busy.cxxCompilerName";
const char CPP_LINKERNAME[] = "busy.linkerName";
const char CPP_PLATFORMCFLAGS[] = "busy.platformCFlags";
const char CPP_PLATFORMCXXFLAGS[] = "busy.platformCxxFlags";
const char CPP_PLATFORMPATH[] = "busy.platformPath";
const char CPP_XCODESDKNAME[] = "busy.xcodeSdkName";
const char CPP_XCODESDKVERSION[] = "busy.xcodeSdkVersion";

// Settings page
const char BUSY_SETTINGS_CATEGORY[]  = "YM.busy";
const char BUSY_SETTINGS_TR_CATEGORY[] = QT_TRANSLATE_NOOP("BusyProjectManager", "Busy");
const char BUSY_SETTINGS_CATEGORY_ICON[]  = ":/projectexplorer/images/build.png";

const char BUSY_PROPERTIES_KEY_FOR_KITS[] = "BusyProjectManager.busy-properties";

} // namespace Constants
} // namespace BusyProjectManager

#endif // BUSYCONSTANTS_H
