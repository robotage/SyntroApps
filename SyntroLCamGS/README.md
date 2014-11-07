# SyntroLCamGS

SyntroLCamGS is a Linux only Syntro application for streaming media from USB webcams
to a SyntroNet system. It can generate MJPEG/PCM, MP4V/MP4A and H.264/MP4A video/audio streams.

Check out www.richards-tech.com for more details.

### Dependencies

1. Qt4 or Qt5 development libraries and headers
2. SyntroCore libraries and headers 
3. gstreamer libraries

### Fetch
To get the gstreamer dependencies:

	sudo apt-get install libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev libgstreamer-plugins-bad0.10-dev

Then get the repo (assumes using the standard location of ~/SyntroNet):

	cd ~/SyntroNet
	git clone git://github.com/richards-tech/SyntroLCamGS.git


### Build 
	
	cd ~/SyntroNet/SyntroLCamGS
	qmake 
	make 
	sudo make install

### Run

#### GUI mode

	SyntroLCamGS


#### Console mode

Use this mode when you have no display on the local system or in the
typical case where you only want to watch the streamed video on a 
remote system using SyntroViewGS or another SyntroNet app.

	SyntroLCamGS -c


Enter h to get some help.


#### Daemon mode

This runs SyntroLCamGS as a background process with no console interaction.

	SyntroLCamGS -c -d

This only works in conjunction with console mode.


### Configuration

SyntroLCamGS writes a configuration file to ~/.config/Syntro/SyntroLCamGS.ini, so if you run it from your home directory like this

        ~$ SyntroLCamGS &

Then you will get a file called ~/.config/Syntro/SyntroLCamGS.ini like this created:

 		[General]
		appName=hex
		appType=SyntroLCamGS
		componentType=SyntroLCamGS
		controlRevert=0
		heartbeatInterval=1
		heartbeatTimeout=5
		localControl=0
		localControlPriority=0
		runtimeAdapter=
		runtimePath=

		[Camera]
		device=0
		format=MJPG
		frameRate=30
		height=480
		streamName=video
		width=640

		[Logging]
		diskLog=true
		logKeep=5
		logLevel=info
		netLog=true

        ....


If you run more then one SyntroLCamGS instance on the same machine then you
must give them distinct settings files with distinct appNames in them. The
settings files can have any name you want. The convention is to give them
a .ini extension. Specify the non-default settings file when launching 
SyntroLCamGS using the -s flag.

Example:

	SyntroLCam -scamera2.ini <other options>


