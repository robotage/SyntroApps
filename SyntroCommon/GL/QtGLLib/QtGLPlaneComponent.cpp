//
//  Copyright (c) 2014 richards-tech
//
//  This file is part of QtGLLib
//
//  QtGLLib is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  QtGLLib is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with QtGLLib.  If not, see <http://www.gnu.org/licenses/>.
//

#include "QtGL.h"

QtGLPlaneComponent::QtGLPlaneComponent()
{
}

QtGLPlaneComponent::~QtGLPlaneComponent()
{

}

void QtGLPlaneComponent::generate(float width, float height)
{
    static const float coords[4][2] = {{+0.5f, +0.5f}, {-0.5f, +0.5f}, {-0.5f, -0.5f}, {+0.5f, -0.5f}};

	reset();

	for (int vert = 0; vert < 4; vert++) {
		addTextureCoord(QVector2D(vert == 0 || vert == 3, vert == 0 || vert == 1));
        addVertex(QVector3D(width * coords[vert][0], height * coords[vert][1], 0));
   }
}

void QtGLPlaneComponent::draw()
{
	QtGLComponent::draw(GL_TRIANGLE_FAN);
}
