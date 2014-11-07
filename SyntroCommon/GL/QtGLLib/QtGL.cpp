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

//	This file contains OpenGL and Qt includes. This file should be included first
//	and the order of includes is very important (especially on Linux).

#include <QtGL.h>

QGLWidget *globalGLWidget;									// the global GL widget
TRANSFORM_MATRICES globalTransforms;						// the various viewport transforms
LIGHT_SOURCES globalLights;									// the lights
QtGLShader *globalShader[QTGLSHADER_COUNT];					// the shaders

void QtGLInit(QGLWidget *widget)
{
	globalLights.lightCount = 0;							// no lights
	globalShader[QTGLSHADER_FLAT] = new QtGLFlatShader(widget);
	globalShader[QTGLSHADER_TEXTURE] = new QtGLTextureShader(widget);
	globalShader[QTGLSHADER_ADS] = new QtGLADSShader(widget);
	globalShader[QTGLSHADER_ADSTEXTURE] = new QtGLADSTextureShader(widget);
}

void QtGLAddLight(const QVector4D& position, const QVector3D& ambient, 
						const QVector3D& diffuse, const QVector3D& specular)
{
	int count;
	if ((count = globalLights.lightCount) == QTGL_MAX_LIGHTS) {
		qDebug() << "Too many lights";
		return;
	}
	globalLights.ambient[count] = ambient;
	globalLights.diffuse[count] = diffuse;
	globalLights.specular[count] = specular;
	globalLights.position[count] = position;
	globalLights.lightCount++;
}

bool QtGLRayRectangleIntersection(QVector3D& intersection, const QVector3D& ray0, const QVector3D& ray1, 
				const QVector3D& plane0, const QVector3D& plane1, const QVector3D& plane2, const QVector3D& plane3,
				bool checkInside)
{
	QVector3D normal;
	QVector3D test;
	float dist0, dist1;

	//	Compute normal to plane

	normal = QVector3D::crossProduct(plane1 - plane0, plane2 - plane0);
	normal.normalize();

	// find distance from points defining line to plane

	dist0 = QVector3D::dotProduct(ray0 - plane0, normal);
	dist1 = QVector3D::dotProduct(ray1 - plane0, normal);

	if (qFuzzyCompare(dist0, dist1))
		return false;										// line and plane are parallel

	intersection = ray0 + (ray1 - ray0) * (-dist0 / (dist1 - dist0));

	if (!checkInside)
		return true;

	// check if intersection point lies within the rectangle

	test = QVector3D::crossProduct(normal, plane1 - plane0);
	if (QVector3D::dotProduct(test, intersection - plane0) < 0.0f)
		return false;

	test = QVector3D::crossProduct(normal, plane2 - plane1);
	if (QVector3D::dotProduct(test, intersection - plane1) < 0.0f)
		return false;

	test = QVector3D::crossProduct(normal, plane3 - plane2);
	if (QVector3D::dotProduct(test, intersection - plane2) < 0.0f)
		return false;

	test = QVector3D::crossProduct(normal, plane0 - plane3);
	if (QVector3D::dotProduct(test, intersection - plane3) < 0.0f)
		return false;

	return true;
}
