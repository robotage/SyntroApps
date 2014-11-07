# SyntroView

SyntroView is Qt GUI application for viewing remote MJPEG/PCM SyntroNet media feeds.

Check out www.richards-tech.com for more details.

### Dependencies

1. Qt4 or Qt5 development libraries and headers
2. SyntroCore libraries and headers, compiled and installed (see repo for details). 

### Build 

To build SyntroView on Linux:

        qmake
        make 
        sudo make install

To build on Windows, use the SyntroView.sln solution file.

### Run

SyntroView can be run from the command line like this

        SyntroView

Single click on a particular window to enable sound for that stream. Double click to get an enlarged via of a stream.

