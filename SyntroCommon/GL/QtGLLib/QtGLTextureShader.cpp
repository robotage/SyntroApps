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

QtGLTextureShader::QtGLTextureShader(QObject *parent)
    : QtGLShader(parent)
{
    #define PROGRAM_VERTEX_ATTRIBUTE 0
    #define PROGRAM_TEXCOORD_ATTRIBUTE 1

    m_type = QTGLSHADER_TEXTURE;

    m_vshader = new QGLShader(QGLShader::Vertex, parent);
    const char *vsrc =
        "attribute highp vec3 vertex;\n"
        "attribute mediump vec2 texCoord;\n"
        "varying mediump vec2 texc;\n"
        "uniform mediump mat4 matrix;\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = matrix * vec4(vertex, 1.0);\n"
        "    texc = texCoord;\n"
        "}\n";
    m_vshader->compileSourceCode(vsrc);

    m_fshader = new QGLShader(QGLShader::Fragment, parent);
    const char *fsrc =
        "uniform sampler2D texture;\n"
        "varying mediump vec2 texc;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragColor = texture2D(texture, texc.st);\n"
        "}\n";
    m_fshader->compileSourceCode(fsrc);

    addShader(m_vshader);
    addShader(m_fshader);
    bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
    bindAttributeLocation("texCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    link();
    setUniformValue("texture", 0);
}

QtGLTextureShader::~QtGLTextureShader()
{

}

void QtGLTextureShader::load(const QVector3D *vertices, const QVector2D *textureCoords)
{
    bind();
    setUniformValue("matrix", globalTransforms.modelViewProjectionMatrix);
    enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, vertices);
    setAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE, textureCoords);
}
