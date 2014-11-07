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

QtGLFlatShader::QtGLFlatShader(QObject *parent)
	: QtGLShader(parent)
{
	#define PROGRAM_VERTEX_ATTRIBUTE 0

	m_type = QTGLSHADER_FLAT;

    m_vshader = new QGLShader(QGLShader::Vertex, parent);
    const char *vsrc =
        "attribute highp vec4 vertex;\n"
        "uniform mediump mat4 matrix;\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = matrix * vertex;\n"
        "}\n";
    m_vshader->compileSourceCode(vsrc);

    m_fshader = new QGLShader(QGLShader::Fragment, parent);
    const char *fsrc =
        "uniform mediump vec4 color;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragColor = color;\n"
        "}\n";
    m_fshader->compileSourceCode(fsrc);

    addShader(m_vshader);
    addShader(m_fshader);
    bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
    link();
    setUniformValue("color", QVector4D(1.0f, 0, 0, 1.0f));
}

QtGLFlatShader::~QtGLFlatShader()
{

}

void QtGLFlatShader::load(const QVector3D *vertices, const QColor& color)
{
    bind();
    setUniformValue("matrix", globalTransforms.modelViewProjectionMatrix);
	setUniformValue("color", color);
    enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, vertices);
 }
