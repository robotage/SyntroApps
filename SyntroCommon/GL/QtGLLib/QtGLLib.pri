#//
#//  Copyright (c) 2014 richards-tech
#//
#//  This file is part of QtGLLib
#//
#//  QtGLLib is free software: you can redistribute it and/or modify
#//  it under the terms of the GNU General Public License as published by
#//  the Free Software Foundation, either version 3 of the License, or
#//  (at your option) any later version.
#//
#//  QtGLLib is distributed in the hope that it will be useful,
#//  but WITHOUT ANY WARRANTY; without even the implied warranty of
#//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#//  GNU General Public License for more details.
#//
#//  You should have received a copy of the GNU General Public License
#//  along with QtGLLib.  If not, see <http://www.gnu.org/licenses/>.
#//

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += $$PWD/QtGL.h \
    $$PWD/QtGLShader.h \
    $$PWD/QtGLTextureShader.h \
    $$PWD/QtGLFlatShader.h \
    $$PWD/QtGLADSShader.h \
    $$PWD/QtGLADSTextureShader.h \
    $$PWD/QtGLComponent.h \
    $$PWD/QtGLPlaneComponent.h \
    $$PWD/QtGLCylinderComponent.h \
    $$PWD/QtGLWireCubeComponent.h \
    $$PWD/QtGLDiskComponent.h \
    $$PWD/QtGLSphereComponent.h
 
SOURCES += $$PWD/QtGL.cpp \
    $$PWD/QtGLTextureShader.cpp \
    $$PWD/QtGLFlatShader.cpp \
    $$PWD/QtGLADSShader.cpp \
    $$PWD/QtGLADSTextureShader.cpp \
    $$PWD/QtGLComponent.cpp \
    $$PWD/QtGLPlaneComponent.cpp \
    $$PWD/QtGLCylinderComponent.cpp \
    $$PWD/QtGLWireCubeComponent.cpp \
    $$PWD/QtGLDiskComponent.cpp \ 
    $$PWD/QtGLSphereComponent.cpp 
