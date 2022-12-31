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

#ifndef BUSYPROJECTMANAGERCONSTANTS_H
#define BUSYPROJECTMANAGERCONSTANTS_H

#include <QtGlobal>

namespace BusyProjectManager {
namespace Constants {

// Contexts
const char PROJECT_ID[] = "Busy.BusyProject";

// MIME types:
const char MIME_TYPE[] = "text/x-busy-project";
const char LANG[] = "BUSY";

// Actions:
const char ACTION_REPARSE_BUSY[] = "Busy.Reparse";
const char ACTION_REPARSE_BUSY_CONTEXT[] = "Busy.ReparseCtx";
const char ACTION_BUILD_FILE_CONTEXT[] = "Busy.BuildFileCtx";
const char ACTION_BUILD_FILE[] = "Busy.BuildFile";
const char ACTION_BUILD_PRODUCT_CONTEXT[] = "Busy.BuildProductCtx";
const char ACTION_BUILD_PRODUCT[] = "Busy.BuildProduct";
const char ACTION_BUILD_SUBPROJECT_CONTEXT[] = "Busy.BuildSubprojectCtx";
const char ACTION_BUILD_SUBPROJECT[] = "Busy.BuildSubproduct";

// Ids:
const char BUSY_BUILDSTEP_ID[] = "Busy.BuildStep";
const char BUSY_CLEANSTEP_ID[] = "Busy.CleanStep";
const char BUSY_INSTALLSTEP_ID[] = "Busy.InstallStep";
const char BUSY_EDITOR_ID[] = "Busy.Editor";

// BUSY strings:
static const char BUSY_VARIANT_DEBUG[] = "debug";
static const char BUSY_VARIANT_RELEASE[] = "release";

static const char BUSY_CONFIG_VARIANT_KEY[] = "busy.buildVariant";
static const char BUSY_CONFIG_PROFILE_KEY[] = "busy.profile";
#if 0
static const char BUSY_CONFIG_DECLARATIVE_DEBUG_KEY[] = "Qt.declarative.qmlDebugging";
static const char BUSY_CONFIG_QUICK_DEBUG_KEY[] = "Qt.quick.qmlDebugging";
#endif

// Icons:
static const char BUSY_GROUP_ICON[] = ":/busyprojectmanager/images/groups.png";
static const char BUSY_PRODUCT_OVERLAY_ICON[] = ":/busyprojectmanager/images/productgear.png";

} // namespace Constants
} // namespace BusyProjectManager

#endif // BUSYPROJECTMANAGERCONSTANTS_H

