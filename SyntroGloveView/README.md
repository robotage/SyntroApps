# SyntroGloveView

SyntroGloveView is Qt GUI application for viewing remote SyntroGlove data feeds generated by SyntroEdisonGlove (for example).

Check out www.richards-tech.com for more details.

### Dependencies

1. Qt4 or Qt5 development libraries and headers
2. SyntroCore libraries and headers, compiled and installed (see repo for details). 

### Build 

To build SyntroView on Linux:

        qmake
        make 
        sudo make install

### Run

SyntroGloveView can be run from the command line like this

        SyntroGloveView

A few seconds after the window has opened up, the "Select source" button should become available. Click on this to open up a dialog that allows the specific SyntroGlove data stream to be selected. Once this has been done, the selection will be retained until a new source is selected.

