# SyntroLCam

SyntroLCam is a Linux-only SyntroNet application for streaming media from USB webcams
to a SyntroNet system.

Check out www.richards-tech.com for more details.

### Dependencies

1. Qt4 or Qt5 development libraries and headers
2. SyntroCore libraries and headers 


### Build 

	cd ~/SyntroNet/SyntroApps/SyntroLCam
	qmake
	make
	sudo make install


### Run

#### GUI mode

        SyntroLCam


#### Console mode

Use this mode when you have no display on the local system or in the
typical case where you only want to watch the streamed video on a 
remote system using SyntroView or another Syntro app.

        SyntroLCam -c


Enter h to get some help.


#### Daemon mode

This runs SyntroLCam as a background process with no console interaction.

        SyntroLCam -c -d

This only works in conjunction with console mode.


### Configuration

SyntroLCam writes a configuration file to ~/.config/Syntro/SyntroLCam.ini, so if you run it from your home directory like this

        ~$ SyntroLCam &

Then you will get a file called ~/.config/Syntro/SyntroLCam.ini like this created

        scott@hex:~$ cat  SyntroLCam.ini

		[General]
		appName=hex
		appType=SyntroLCam
		componentType=SyntroLCam
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


If you run more then one SyntroLCam instance on the same machine then you
must give them distinct settings files with distinct appNames in them. The
settings files can have any name you want. The convention is to give them
a .ini extension. Specify the non-default settings file when launching 
SyntroLCam using the -s flag.

Example:

		SyntroLCam -scamera2.ini <other options>


