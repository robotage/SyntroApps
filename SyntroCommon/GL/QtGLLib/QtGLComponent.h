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

//	Based on code from the OpenGL SuperBible:

/*
 *  OpenGL SuperBible
 *
Copyright (c) 2007-2009, Richard S. Wright Jr.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list 
of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this list 
of conditions and the following disclaimer in the documentation and/or other 
materials provided with the distribution.

Neither the name of Richard S. Wright Jr. nor the names of other contributors may be used 
to endorse or promote products derived from this software without specific prior 
written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR 
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#ifndef QTGLCOMPONENT_H
#define QTGLCOMPONENT_H

class QtGLComponent
{
public:
	QtGLComponent();
	~QtGLComponent();

	virtual void setShader(QtGLShader *shader);
	virtual void setTexture(GLuint texture);
	virtual void setTexture(const QImage& image);
	virtual void setMaterial(const COMPONENT_MATERIAL& material);
	virtual void setMaterial(const QVector3D& ambient, const QVector3D& diffuse, const QVector3D& specular,
			float shininess);
	virtual void draw(GLenum drawMode);

	virtual void setColor(const QColor& color);				// sets color for flat shader

	inline QVector3D& getBoundMinus() { return m_boundMinus; };
	inline QVector3D& getBoundPlus() { return m_boundPlus; };

protected:
	void reset();											// clears everything and starts again
	int nearestPOT(int val);								// returns nearest power of 2
	void addTriangle(QVector3D *verts, QVector3D *norms, QVector2D *texCoords); // adds a triangle
	void addVertex(const QVector3D& vertex);				// adds a vertex to the component
	void addNormal(const QVector3D& normal);				// adds a normal to the component
	void addTextureCoord(const QVector2D& textureCoord);	// adds a texture coord to the component
	void addIndex(GLushort index);							// adds an index to the component
	void useIndexBuffer(bool use);							// says that index buffer is to be used in draw
	void useNormalBuffer(bool use);							// says that normal buffer is to be used in draw


	void updateBoundingBox(const QVector3D& vert);			// update the component bounding box

private:
	QtGLShader *m_shader;
	int m_texture;			
	QColor m_color;
	COMPONENT_MATERIAL m_material;

	QVector <QVector3D> m_vertices;
	QVector <QVector3D> m_normals;
	QVector <QVector2D> m_textureCoords;
	QVector <GLushort> m_indices;

	QGLBuffer *m_indexBuffer;
	QGLBuffer *m_normalBuffer;

	bool m_useIndexBuffer;
	bool m_useNormalBuffer;

	QVector3D m_boundMinus;									// lowest in each coord for box
	QVector3D m_boundPlus;									// highest in each coord for box


};
		
#endif // QTGLCOMPONENT_H
