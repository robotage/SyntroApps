# SyntroLCamGS

### Build 
	
	cd ~/richards-tech/SyntroLCamGS
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

	SyntroLCamGS -scamera2.ini <other options>


