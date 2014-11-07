# SyntroReiew

SyntroReview is Qt GUI application for viewing offline SyntroNet media streams captured by SyntroDB (part of SyntroCore).

Check out www.richards-tech.com for more details.


### Dependencies

1. Qt4 or Qt5 development libraries and headers
2. SyntroCore libraries and headers, compiled and installed. 


### Fetch

        git clone git://github.com/richards-tech/SyntroReview.git


### Build 

On Linux:

        qmake 
        make 
        sudo make install

On Windows, use the provided VS2010 solution file to build.

### Run

The executable file is called SyntroReview and can be run from the
command line like this

        SyntroReview

The first thing to do is to select the available SyntroDBs as required. Once SyntroReview has
connected to a SyntroControl (there must be at least one SyntroControl per LAN - SyntroView runs one internally by default),
use Actions > Cloud File System Selection. The resulting dialog allows SyntroDBs to be selected and added to the current list of
Cloud File System (CFS) sources.

When the selected SyntroDBs have supplied their file directories to SyntroReview, the File > Open option will become available. This will display a list of available media files. Select one to play. The slider and other controls can then be used to eithe play the stream or else select specific frames for review. Individual jpeg images can also be saved using the Save Frame button.

