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

#ifndef BUSYPROJECTMANAGER_GLOBAL_H
#define BUSYPROJECTMANAGER_GLOBAL_H

#include <qglobal.h>

#ifdef LEANCREATOR_ALL_STATIC
#  define BUSYPROJECTMANAGER_EXPORT
#else
#if defined(BUSYPROJECTMANAGER_LIBRARY)
#  define BUSYPROJECTMANAGER_EXPORT Q_DECL_EXPORT
#else
#  define BUSYPROJECTMANAGER_EXPORT Q_DECL_IMPORT
#endif
#endif

#endif // BUSYPROJECTMANAGER_GLOBAL_H
