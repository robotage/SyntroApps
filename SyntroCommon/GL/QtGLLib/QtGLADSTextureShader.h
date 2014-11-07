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

#ifndef QTGLADSTEXTURESHADER_H
#define QTGLADSTEXTURESHADER_H

class QtGLADSTextureShader : public QtGLShader
{
public:
	QtGLADSTextureShader(QObject *parent);
	~QtGLADSTextureShader();

	void load(const QVector3D *vertices, const QVector3D *normals, 
			const QVector2D *textureCoords, const COMPONENT_MATERIAL& material);

private:

};
		
#endif // QTGLADSTEXTURESHADER_H
