#
#  Copyright (c) 2014 Richard Barnett
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
TEMPLATE = app
TARGET = SyntroRTSP
target.path = /usr/bin
INSTALLS += target
DESTDIR = Output
QT += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += debug_and_release link_pkgconfig
PKGCONFIG += syntro
IS_TK1 = $$(TK1)

contains(IS_TK1, "TK1") {
    message(Using gstreamer-0.10)
    DEFINES += USE_GST010
    PKGCONFIG += gstreamer-0.10 gstreamer-app-0.10
} else {
    message(Using gstreamer-1.0)
    DEFINES += USE_GST10
    PKGCONFIG += gstreamer-1.0 gstreamer-app-1.0
}
macx:CONFIG -= app_bundle
DEFINES += QT_NETWORK_LIB
INCLUDEPATH += GeneratedFiles
DEPENDPATH +=
MOC_DIR += GeneratedFiles/release
OBJECTS_DIR += release
UI_DIR += GeneratedFiles
RCC_DIR += GeneratedFiles
include(SyntroRTSP.pri)
