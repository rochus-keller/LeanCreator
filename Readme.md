![icon](http://software.rochus-keller.ch/creator-logo-100x460.png)

## Welcome to LeanCreator - a lean version of Qt Creator

NOTE that this project is in an early stage and there is no MVP yet.

LeanCreator is supposed to be a stripped-down version of Qt Creator, which includes the essential features for C++ (and possibly Oberon+ and Verilog) development and is easy to build from source on all platforms supported by LeanQt.

LeanCreator uses LeanQt instead of the original Qt toolkit and is built using BUSY instead of qmake.

LeanCreator is based on the code of [Qt Creator 3.6.1](https://download.qt.io/archive/qtcreator/3.6/3.6.1/qt-creator-opensource-src-3.6.1.tar.gz), mostly because I already know the internals due to [VerilogCreator](https://github.com/rochus-keller/VerilogCreator/) and it's [QtcVerilog](https://github.com/rochus-keller/QtcVerilog/) IDE, and because I'm still using Qt Creator 3.4 most of the time which is great and does everything I need.

### Planned or work-in-progress features

Successfully tested so far on Linux x86.

- [x] Basic, stand-alone application with statically linked plugins
- [x] C++ support
- [x] rebranding
- [ ] deep BUSY integration (instead of qmake)
- [ ] GCC, Clang and MSVC support
- [ ] GDB and CDB support
- [ ] Lua automation (instead of JS)
- [ ] Designer integration

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

### How to build LeanCreator

1. Create a new directory; we call it the root directory here
1. Download https://github.com/rochus-keller/LeanQt/archive/refs/heads/master.zip and unpack it to the root directory; rename the resulting directory to "LeanQt".
1. Download https://github.com/rochus-keller/LeanCreator/archive/refs/heads/master.zip and unpack it to the root directory; rename the resulting directory to "LeanCreator".
1. Download https://github.com/rochus-keller/BUSY/archive/refs/heads/master.zip and unpack it to the root directory; rename the resulting directory to "build".
1. Open a command line in the build directory and type `cc *.c -O2 -lm -O2 -o lua` or `cl /O2 /MD /Fe:lua.exe *.c` depending on whether you are on a Unix or Windows machine; wait a few seconds until the Lua executable is built.
1. Now type `./lua build.lua ../LeanCreator` (or `lua build.lua ../LeanCreator` on Windows); wait a few minutes until the leancreator executable is built; you find it in the output/app subdirectory.

### Additional Credits

- Copyright (C) 2016 by The Qt Company Ltd. 
- Copyright (C) 2008-2011 Nokia Corporation and/or its subsidiary(-ies).
- Copyright (C) 2007 Trolltech AS.

### Support

If you need support or would like to post issues or feature requests please use the Github issue list at https://github.com/rochus-keller/LeanCreator/issues or send an email to the author.

