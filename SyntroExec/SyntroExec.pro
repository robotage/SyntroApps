#//
#//  Copyright (c) 2014 richards-tech.
#//
#//  This file is part of SyntroNet
#//
#//  SyntroNet is free software: you can redistribute it and/or modify
#//  it under the terms of the GNU General Public License as published by
#//  the Free Software Foundation, either version 3 of the License, or
#//  (at your option) any later version.
#//
#//  SyntroNet is distributed in the hope that it will be useful,
#//  but WITHOUT ANY WARRANTY; without even the implied warranty of
#//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#//  GNU General Public License for more details.
#//
#//  You should have received a copy of the GNU General Public License
#//  along with SyntroNet.  If not, see <http://www.gnu.org/licenses/>.
#//

TEMPLATE = app
TARGET = SyntroExec
DESTDIR = Output
QT += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += debug_and_release link_pkgconfig

unix {
        macx {
                LIBS += /usr/local/lib/libSyntroLib.dylib \
                        /usr/local/lib/libSyntroGUI.dylib \
                        /usr/local/lib/libSyntroControlLib.dylib

                QT += multimedia

                INCLUDEPATH += /usr/local/include/syntro \
                                /usr/local/include/syntro/SyntroControlLib \
                                /usr/local/include/syntro/SyntroAV

                target.path = /usr/local/bin
        }
        else {
                CONFIG += link_pkgconfig
                PKGCONFIG += syntro

                target.path = /usr/bin
        }

        INSTALLS += target
}


DEFINES += QT_NETWORK_LIB
INCLUDEPATH += GeneratedFiles
DEPENDPATH +=
MOC_DIR += GeneratedFiles/release
OBJECTS_DIR += release
UI_DIR += GeneratedFiles
RCC_DIR += GeneratedFiles
include(SyntroExec.pri)

