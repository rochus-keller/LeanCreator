/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2023 Rochus Keller (me@rochus-keller.ch) for LeanCreator
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

#ifndef PROJECTEXPLORER_ABI_H
#define PROJECTEXPLORER_ABI_H

#include "projectexplorer_export.h"

#include <QList>
#include <QHash>

namespace Utils { class FileName; }

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// ABI (documentation inside)
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT Abi
{
public:
    enum Architecture {
        ArmArchitecture,
        X86Architecture,
        ItaniumArchitecture,
        MipsArchitecture,
        PowerPCArchitecture,
        ShArchitecture,
        UnknownArchitecture
    };

    enum OS {
        BsdOS,
        LinuxOS,
        MacOS,
        UnixOS,
        WindowsOS,
        VxWorks,
        UnknownOS
    };

    enum OSFlavor {
        // BSDs
        FreeBsdFlavor,
        NetBsdFlavor,
        OpenBsdFlavor,

        // Linux
        GenericLinuxFlavor,
        AndroidLinuxFlavor,

        // Mac
        GenericMacFlavor,

        // Unix
        GenericUnixFlavor,
        SolarisUnixFlavor,

        // Windows
        WindowsMsvc2005Flavor,
        WindowsMsvc2008Flavor,
        WindowsMsvc2010Flavor,
        WindowsMsvc2012Flavor,
        WindowsMsvc2013Flavor,
        WindowsMsvc2015Flavor,
        WindowsMsvc2017Flavor,
        WindowsMsvc2019Flavor,
        WindowsMsvc2022Flavor,
        WindowsLastMsvcFlavor = WindowsMsvc2022Flavor,
        WindowsMSysFlavor,
        WindowsCEFlavor,

        VxWorksFlavor,

        UnknownFlavor
    };

    enum BinaryFormat {
        ElfFormat,
        MachOFormat,
        PEFormat,
        RuntimeQmlFormat,
        UnknownFormat
    };

    Abi() :
        m_architecture(UnknownArchitecture), m_os(UnknownOS),
        m_osFlavor(UnknownFlavor), m_binaryFormat(UnknownFormat), m_wordWidth(0)
    { }

    Abi(const Architecture &a, const OS &o,
        const OSFlavor &so, const BinaryFormat &f, unsigned char w);
    Abi(const QString &abiString);

    static Abi abiFromTargetTriplet(const QString &machineTriple);

    bool operator != (const Abi &other) const;
    bool operator == (const Abi &other) const;
    bool isCompatibleWith(const Abi &other) const;

    bool isValid() const;
    bool isNull() const;

    Architecture architecture() const { return m_architecture; }
    OS os() const { return m_os; }
    OSFlavor osFlavor() const { return m_osFlavor; }
    BinaryFormat binaryFormat() const { return m_binaryFormat; }
    unsigned char wordWidth() const { return m_wordWidth; }

    QString toString() const;

    static QString toString(const Architecture &a);
    static QString toString(const OS &o);
    static QString toString(const OSFlavor &of);
    static QString toString(const BinaryFormat &bf);
    static QString toString(int w);

    static QList<OSFlavor> flavorsForOs(const OS &o);

    static Abi hostAbi();
    static QList<Abi> abisOfBinary(const Utils::FileName &path);

private:
    Architecture m_architecture;
    OS m_os;
    OSFlavor m_osFlavor;
    BinaryFormat m_binaryFormat;
    unsigned char m_wordWidth;
};

inline int qHash(const ProjectExplorer::Abi &abi)
{
    int h = abi.architecture()
            + (abi.os() << 3)
            + (abi.osFlavor() << 6)
            + (abi.binaryFormat() << 10)
            + (abi.wordWidth() << 13);
    return QT_PREPEND_NAMESPACE(qHash)(h);
}
} // namespace ProjectExplorer

#endif // PROJECTEXPLORER_ABI_H
