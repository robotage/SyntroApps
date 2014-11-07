# SyntroRTSP

SyntroRTSP is a Linux-only SyntroNet application for streaming media from H.264 Foscam IP cameras
to a SyntroNet system. This code has been tested with the Foscam FI9821W V2 camera.

Check out www.richards-tech.com for more details.

### Dependencies

1. Qt4 or Qt5 development libraries and headers
2. SyntroCore libraries and headers 
3. gstreamer1.0

To get the gstreamer dependencies:

	sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev	

### Build 

	cd ~/SyntroNet/SyntroApps/SyntroRTSP
	qmake
	make
	sudo make install

### Run

#### GUI mode

        SyntroRTSP


#### Console mode

Use this mode when you have no display on the local system or in the
typical case where you only want to watch the streamed video on a 
remote system using SyntroView or another Syntro app.

        SyntroRTSP -c


Enter h to get some help.


#### Daemon mode

This runs SyntroRTSP as a background process with no console interaction.

        SyntroRTSP -c -d

This only works in conjunction with console mode.


### Configuration

SyntroRTSP writes a configuration file to ~/.config/Syntro/SyntroRTSP.ini. This can be edited with any editor or modified using dialogs when running in GUI mode.

If you run more then one SyntroRTSP instance on the same machine then you
must give them distinct settings files with distinct appNames in them. The
settings files can have any name you want. The convention is to give them
a .ini extension. Specify the non-default settings file when launching 
SyntroRTSP using the -s flag.

Example:

		SyntroRTSP -scamera2.ini <other options>


