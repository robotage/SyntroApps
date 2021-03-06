//
//  Copyright (c) 2014 Scott Ellis and Richard Barnett
//	
//  This file is part of SyntroNet
//
//  SyntroNet is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  SyntroNet is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with SyntroNet.  If not, see <http://www.gnu.org/licenses/>.
//

#include "SyntroView.h"
#include <qapplication.h>

#include "SyntroUtils.h"


int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	SyntroUtils::loadStandardSettings(PRODUCT_TYPE, a.arguments());
#ifdef USING_GSTREAMER
    gst_init (&argc, &argv);
#endif
    SyntroView *w = new SyntroView();

	w->show();

	return a.exec();
}
