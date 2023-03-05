![icon](http://software.rochus-keller.ch/creator-logo-100x460.png)

## Welcome to LeanCreator - a lean version of Qt Creator

LeanCreator is a stripped-down version of Qt Creator, which includes the essential features for C and C++ development, and is easy to build from source on all platforms supported by LeanQt.

LeanCreator uses [LeanQt](https://github.com/rochus-keller/LeanQt) instead of the original Qt toolkit, and is built with and uses the [BUSY build system](https://github.com/rochus-keller/BUSY) instead of qmake.

In contrast to QtCreator, LeanCreator is a single file application, easy to download and install; there are no separate shared libraries or support files or directories. Precompiled binaries are available (see below), but it's also easy to build LeanCreator from scratch with no other requirements than a C++11 compatible compiler.

LeanCreator is based on the code of [Qt Creator 3.6.1](https://download.qt.io/archive/qtcreator/3.6/3.6.1/qt-creator-opensource-src-3.6.1.tar.gz), mostly because I already know the internals due to [VerilogCreator](https://github.com/rochus-keller/VerilogCreator/) and it's [QtcVerilog](https://github.com/rochus-keller/QtcVerilog/) IDE, and because I'm still using Qt Creator 3.4 most of the time which does everything I need and is even faster than later versions.

In combination with LeanQt this is a big codebase and as such also a good test case for the BUSY build system; the [cloc tool](http://cloc.sourceforge.net) reports 351 kSLOC for LeanCreator and 993 kSLOC for LeanQt (1.34 mmSLOC in total).

Here is a screenshot:

![LeanCreator Screenshot](http://software.rochus-keller.ch/leancreator-2023-02-22-screenshot.png)


### Planned or work-in-progress features

The current version was successfully compiled and tested on Linux x86 & x86_64, macOS x86_64 & M1, and Windows x86 & AMD64. The tests included navigating the LeanQt source tree with different parameter settings, and running some of the examples in the debugger with breakpoints and value inspections. 
Note that there is an issue with LLDB on Mac M1 (see https://github.com/rochus-keller/LeanCreator/issues/1).

- [x] Basic, stand-alone application with statically linked plugins
- [x] C++ support
- [x] rebranding
- [x] deep BUSY integration (replacement for qmake)
- [x] GCC, Clang and MSVC support 
- [x] GDB, LLDB and CDB support 
- [x] Extend BUSY file navigation
- [x] BUSY multi-core parallel builder
- [ ] BUSY builds with header dependency tracking
- [ ] Wizzards for project and code file creation
- [ ] Other convenience features
- [ ] Ninja builds

### Long term plan

- [ ] Display BUSY file calculated values, gray-out inactives
- [ ] Lua automation
- [ ] Designer integration
- [ ] Oberon+, Verilog and Lola integration


### No support planned

- qmake
- qml, quick, JS, Python
- animation or graphics effects, all stuff not supported by LeanQt
- modeleditor
- remote access via ssh
- android, ios, qnx and winrt plugins
- make, autotools, cmake and qbs 
- version control plugins
- emacs or vim simulation

#### Precompiled versions

The following precompiled versions are available at this time:

- [Windows x86](http://software.rochus-keller.ch/leancreator_windows_x86.zip)
- [Windows x86_64](http://software.rochus-keller.ch/leancreator_windows_x64.zip)
- [Linux x86](http://software.rochus-keller.ch/leancreator_linux_x86.tar.gz)
- [Linux x86_64](http://software.rochus-keller.ch/leancreator_linux_x64.tar.gz)
- [Mac x86_64](http://software.rochus-keller.ch/leancreator_macos_x64.zip)
- [Mac M1](http://software.rochus-keller.ch/leancreator_macos_m1.zip)

Just download, unzip and run; no installation required; it's just a single executable.

On Mac the terminal opens when LeanCreator is run, and the menus are only active if the application was in the background one time; to avoid this the application can be included in an application bundle. Also note that the application on Mac must be started via the "open" command from the context menu; otherwise the system refuses to start the app.

NOTE that the Windows versions are compiled with MT using a statically linked C/C++ runtime, so no Microsoft runtime has to be installed. The executable runs even on Windows 7 (Windows XP though doesn't recognize the executable format).

### How to build LeanCreator

To build LeanCreator using LeanQt and the BUSY build system (with no other dependencies than a C++11 compiler), do the following:

1. Create a new directory; we call it the root directory here
1. Download https://github.com/rochus-keller/LeanQt/archive/refs/heads/master.zip and unpack it to the root directory; rename the resulting directory to "LeanQt".
1. Download https://github.com/rochus-keller/LeanCreator/archive/refs/heads/master.zip and unpack it to the root directory; rename the resulting directory to "LeanCreator".
1. Download https://github.com/rochus-keller/BUSY/archive/refs/heads/master.zip and unpack it to the root directory; rename the resulting directory to "BUSY".
1. Open a command line in the build directory and type `cc *.c -O2 -lm -O2 -o lua` or `cl /O2 /MD /Fe:lua.exe *.c` depending on whether you are on a Unix or Windows machine; wait a few seconds until the Lua executable is built.
1. Now type `./lua build.lua ../LeanCreator` (or `lua build.lua ../LeanCreator` on Windows); wait until the LeanCreator executable is built (about an hour); you find it in the output/app subdirectory.

NOTE that if you build on Windows you have to first open a console and run vcvars32.bat or vcvars64.bat provided e.g. by VisualStudio (see e.g. [here](https://learn.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-170) for more information).

### Additional Credits

- Copyright (C) 2016 by The Qt Company Ltd. 
- Copyright (C) 2008-2011 Nokia Corporation and/or its subsidiary(-ies).
- Copyright (C) 2007 Trolltech AS.

### Support

If you need support or would like to post issues or feature requests please use the Github issue list at https://github.com/rochus-keller/LeanCreator/issues or send an email to the author.

