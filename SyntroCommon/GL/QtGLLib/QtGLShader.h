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

#ifndef QTGLSHADER_H
#define QTGLSHADER_H

typedef enum
{
	QTGLSHADER_FLAT,
	QTGLSHADER_TEXTURE,
	QTGLSHADER_ADS,
	QTGLSHADER_ADSTEXTURE,
	QTGLSHADER_COUNT
} QTGLSHADER_TYPE;

class QtGLShader : public QGLShaderProgram
{
public:
	QtGLShader(QObject *parent) : QGLShaderProgram(parent) {};
	virtual ~QtGLShader() { removeAllShaders(); };

	inline QTGLSHADER_TYPE getType() {return m_type;};

//	virtual void load(const QVector3D *vertices, const QVector2D *textureCoords);
	virtual void load(const QVector3D *, const QVector2D *) { qDebug() << "No shader load";};

//	virtual void load(const QVector3D *vertices, const QColor& color);
	virtual void load(const QVector3D *, const QColor&) {qDebug() << "No shader load";};

//	virtual void load(const QVector3D *vertices, const QVector3D *normals, const COMPONENT_MATERIAL& material);
	virtual void load(const QVector3D *, const QVector3D *, const COMPONENT_MATERIAL& ) {qDebug() << "No shader load";};

//	virtual void load(const QVector3D *vertices, const QVector3D *normals, 
//			const QVector2D *textureCoords, const COMPONENT_MATERIAL& material);
	virtual void load(const QVector3D *, const QVector3D *, const QVector2D*, const COMPONENT_MATERIAL& ) {qDebug() << "No shader load";};

protected:
	QGLShader *m_vshader;
	QGLShader *m_fshader;
	QTGLSHADER_TYPE m_type;
};
		
#endif // QTGLSHADER_H
