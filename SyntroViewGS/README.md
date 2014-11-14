# SyntroViewGS

SyntroViewGS is Qt GUI application for viewing remote H.264/MP4A and MJPEG/PCM SyntroNet media feeds. SyntroViewGS automatically detects which is in use and can display a mix of MJPEG/PCM and H.264/MP4A streams in a seamless fashion.

Check out www.richards-tech.com for more details.

### Dependencies

1. Qt4 or Qt5 development libraries and headers
2. SyntroCore libraries and headers, compiled and installed (see repo for details). 
3. gstreamer libraries

To get the gstreamer dependencies on Linux:

	sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev
	sudo apt-get install gstreamer1.0-libav

On Windows, install the gstreamer SDK from in c:\gstreamer-sdk. Instructions are here - http://docs.gstreamer.com/display/GstSDK/Installing+on+Windows.


### Build 

To build SyntroViewGS on Linux:

	cd ~/richards-tech/SyntroApps/SyntroViewGS
	qmake
	make
	sudo make install

To build on Windows, use the SyntroViewGS.sln solution file. It assumes that the gstreamer sdk (http://docs.gstreamer.com/display/GstSDK/Installing+on+Windows) has been installed in the standard location.

### Run

SyntroViewGS can be run from the command line like this

        SyntroViewGS

Single click on a particular window to enable sound for that stream. Double click to get an enlarged via of a stream.

