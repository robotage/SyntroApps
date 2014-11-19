#
#  Copyright (c) 2014 Scott Ellis and Richard Barnett
#
#  This file is part of SyntroNet
#
#  SyntroNet is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  SyntroNet is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with SyntroNet.  If not, see <http://www.gnu.org/licenses/>.
#

DEFINES += USING_GSTREAMER
equals(QT_ARCH, "arm") {
        PKGCONFIG += gstreamer-0.10 gstreamer-app-0.10
    } else {
        DEFINES += USING_GSTREAMER USE_GST10
        PKGCONFIG += gstreamer-1.0 gstreamer-app-1.0
    }

greaterThan(QT_MAJOR_VERSION, 4): cache()

TEMPLATE = app
TARGET = SyntroViewGS

win32* {
        DESTDIR = Release
    } else {
        DESTDIR = Output
    }

QT += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += debug_and_release

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
        } else {
                CONFIG += link_pkgconfig
                PKGCONFIG += syntro
                LIBS += -lasound
                target.path = /usr/bin
        }

        INSTALLS += target
}

DEFINES += QT_NETWORK_LIB

INCLUDEPATH += GeneratedFiles ../SyntroCommon/AVMux

win32-g++:LIBS += -L"$(SYNTRODIR)/bin"

win32-msvc*:LIBS += -L"$(SYNTRODIR)/lib"

win32 {
        DEFINES += _CRT_SECURE_NO_WARNINGS
        INCLUDEPATH += $(SYNTRODIR)/include
        LIBS += -lSyntroLib -lSyntroGUI
}

MOC_DIR += GeneratedFiles/GS/moc

OBJECTS_DIR += objects/GS

UI_DIR += GeneratedFiles/GS

RCC_DIR += GeneratedFiles/GS

include(SyntroViewGS.pri)
