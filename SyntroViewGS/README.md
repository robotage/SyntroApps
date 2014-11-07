# SyntroViewGS

SyntroViewGS is Qt GUI application for viewing remote H.264/MP4A and MJPEG/PCM SyntroNet media feeds. SyntroViewGS automatically detects which is in use and can display a mix of MJPEG/PCM and H.264/MP4A streams in a seamless fashion.

Check out www.richards-tech.com for more details.

### Dependencies

1. Qt4 or Qt5 development libraries and headers
2. SyntroCore libraries and headers, compiled and installed (see repo for details). 
3. gstreamer libraries

To get the gstreamer dependencies on Linux:

	sudo apt-get install libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev libgstreamer-plugins-bad0.10-dev	

On Windows, install the gstreamer SDK from in c:\gstreamer-sdk. Instructions are here - http://docs.gstreamer.com/display/GstSDK/Installing+on+Windows.


### Build 

To build SyntroViewGS on Linux:

	cd ~/SyntroNet/SyntroApps/SyntroViewGS
	qmake
	make
	sudo make install

To build on Windows, use the SyntroViewGS.sln solution file.

### Run

SyntroViewGS can be run from the command line like this

        SyntroViewGS

Single click on a particular window to enable sound for that stream. Double click to get an enlarged via of a stream.

