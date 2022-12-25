/**************************************************************************
**
** Copyright (C) 2015 Denis Mingulov
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

#ifndef CLASSVIEWNAVIGATIONWIDGETFACTORY_H
#define CLASSVIEWNAVIGATIONWIDGETFACTORY_H

#include <core/inavigationwidgetfactory.h>

namespace ClassView {
namespace Internal {

class NavigationWidgetFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT

public:
    NavigationWidgetFactory();

    //! \implements Core::INavigationWidgetFactory::createWidget
    Core::NavigationView createWidget();

    //! \implements Core::INavigationWidgetFactory::saveSettings
    void saveSettings(int position, QWidget *widget);

    //! \implements Core::INavigationWidgetFactory::restoreSettings
    void restoreSettings(int position, QWidget *widget);
};

} // namespace Internal
} // namespace ClassView

#endif // CLASSVIEWNAVIGATIONWIDGETFACTORY_H
