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

QtGLWireCubeComponent::QtGLWireCubeComponent()
{
}

QtGLWireCubeComponent::~QtGLWireCubeComponent()
{

}

void QtGLWireCubeComponent::generate(float width, float height, float depth)
{
	float w2 = width / 2.0f;
	float h2 = height / 2.0f;
	float d2 = depth / 2.0f;

	reset();

	addVertex(QVector3D(-w2, h2, d2));
	addVertex(QVector3D(w2, h2, d2));
	addVertex(QVector3D(-w2, -h2, d2));
	addVertex(QVector3D(w2, -h2, d2));

	addVertex(QVector3D(w2, -h2, d2));
	addVertex(QVector3D(w2, h2, d2));
	addVertex(QVector3D(-w2, -h2, d2));
	addVertex(QVector3D(-w2, h2, d2));

	addVertex(QVector3D(-w2, h2, -d2));
	addVertex(QVector3D(w2, h2, -d2));
	addVertex(QVector3D(-w2, -h2, -d2));
	addVertex(QVector3D(w2, -h2, -d2));

	addVertex(QVector3D(w2, -h2, -d2));
	addVertex(QVector3D(w2, h2, -d2));
	addVertex(QVector3D(-w2, -h2, -d2));
	addVertex(QVector3D(-w2, h2, -d2));

	addVertex(QVector3D(-w2, -h2, -d2));
	addVertex(QVector3D(-w2, -h2, d2));
	addVertex(QVector3D(w2, -h2, -d2));
	addVertex(QVector3D(w2, -h2, d2));

	addVertex(QVector3D(-w2, h2, -d2));
	addVertex(QVector3D(-w2, h2, d2));
	addVertex(QVector3D(w2, h2, -d2));
	addVertex(QVector3D(w2, h2, d2));

}

void QtGLWireCubeComponent::draw()
{
	QtGLComponent::draw(GL_LINES);
}
