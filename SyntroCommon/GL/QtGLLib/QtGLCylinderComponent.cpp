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

#include "QtGL.h"

QtGLCylinderComponent::QtGLCylinderComponent()
{

}

QtGLCylinderComponent::~QtGLCylinderComponent()
{

}

void QtGLCylinderComponent::generate(GLfloat baseRadius, GLfloat topRadius, GLfloat length, GLint numSlices, GLint numStacks)
{
    float radiusStep = (topRadius - baseRadius) / float(numStacks);

	GLfloat stepSizeSlice = (3.1415926536f * 2.0f) / float(numSlices);

	QVector3D vertex[4];
	QVector3D normal[4];
	QVector2D texture[4];

    reset();

    GLfloat ds = 1.0f / float(numSlices);
	GLfloat dt = 1.0f / float(numStacks);
	GLfloat s;
	GLfloat t;

	int vertA = 0;
	int vertB = 1;
	int vertC = 2;
	int vertD = 3;

	for (int i = 0; i < numStacks; i++) {
		if(i == 0)			
			t = 1.0f;
		else
			t = 1.0f - float(i) * dt;

		float tNext;
		if(i == (numStacks - 1))
			tNext = 0.0f;
		else
			tNext = 1.0f - float(i+1) * dt;

		float currentRadius = baseRadius + (radiusStep * float(i));
		float nextRadius = baseRadius + (radiusStep * float(i+1));
		float theta;
		float thetaNext;

		float currentZ = float(i) * (length / float(numStacks)); 
		float nextZ = float(i+1) * (length / float(numStacks));
		
		float zNormal = 0.0f;
		if(!qFuzzyCompare(baseRadius - topRadius, 0.0f)) {
			// Rise over run...
			zNormal = (baseRadius - topRadius);
		}
		
		for (int j = 0; j < numSlices; j++) {		
			if(j == 0)
				s = 1.0f;
			else
				s = 1.0f - float(j) * ds;

			float sNext;
			if(j == (numSlices -1))
				sNext = 0.0f;
			else
				sNext = 1.0f - float(j+1) * ds; 


			theta = stepSizeSlice * float(j);
			if(j == (numSlices - 1))
				thetaNext = 0.0f;
			else
				thetaNext = stepSizeSlice * (float(j+1));
				
			// Inner First
			vertex[vertB].setX(cos(theta) * currentRadius);		// X	
			vertex[vertB].setY(sin(theta) * currentRadius);		// Y
			vertex[vertB].setZ(currentZ);						// Z
			
			normal[vertB].setX(vertex[vertB].x());					// Surface Normal, same for everybody
			normal[vertB].setY(vertex[vertB].y());
			normal[vertB].setZ(zNormal);
			normal[vertB].normalize();
				
			texture[vertB].setX(s);								// Texture Coordinates, I have no idea...
			texture[vertB].setY(t);
	
			// Outer First
			vertex[vertA].setX(cos(theta) * nextRadius);		// X	
			vertex[vertA].setY(sin(theta) * nextRadius);		// Y
			vertex[vertA].setZ(nextZ);							// Z
			
			if(!qFuzzyCompare(nextRadius, 0.0f)) {
				normal[vertA].setX(vertex[vertA].x());			// Surface Normal, same for everybody
				normal[vertA].setY(vertex[vertA].y());			// For cones, tip is tricky
				normal[vertA].setZ(zNormal);
				normal[vertA].normalize();
			}
			else {
				normal[vertA] = normal[vertB];
			}
			
			texture[vertA].setX(s);								// Texture Coordinates, I have no idea...
			texture[vertA].setY(tNext);
			
			// Inner second
			vertex[vertD].setX(cos(thetaNext) * currentRadius);	// X	
			vertex[vertD].setY(sin(thetaNext) * currentRadius);	// Y
			vertex[vertD].setZ(currentZ);						// Z
			
			normal[vertD].setX(vertex[vertD].x());					// Surface Normal, same for everybody
			normal[vertD].setY(vertex[vertD].y());
			normal[vertD].setZ(zNormal);
			normal[vertD].normalize();

			texture[vertD].setX(sNext);							// Texture Coordinates, I have no idea...
			texture[vertD].setY(t);

			// Outer second
			vertex[vertC].setX(cos(thetaNext) * nextRadius);	// X	
			vertex[vertC].setY(sin(thetaNext) * nextRadius);	// Y
			vertex[vertC].setZ(nextZ);							// Z
			
			if(!qFuzzyCompare(nextRadius, 0.0f)) {
				normal[vertC].setX(vertex[vertC].x());			// Surface Normal, same for everybody
				normal[vertC].setY(vertex[vertC].y());
				normal[vertC].setZ(zNormal);
				normal[vertC].normalize();
			}
			else {
				normal[vertC] = normal[vertD];
			}

			texture[vertC].setX(sNext);							// Texture Coordinates, I have no idea...
			texture[vertC].setY(tNext);
		
			addTriangle(vertex, normal, texture);			
			
			// Rearrange for next triangle
			vertex[vertA] = vertex[vertB];
			normal[vertA] = normal[vertB];
			texture[vertA] = texture[vertB];

			vertex[vertB] = vertex[vertD];
			normal[vertB] = normal[vertD];
			texture[vertB] = texture[vertD];
		
			addTriangle(vertex, normal, texture);			
		}
	}
	useIndexBuffer(true);
	useNormalBuffer(true);
}

void QtGLCylinderComponent::draw()
{
	QtGLComponent::draw(GL_TRIANGLES);
}
