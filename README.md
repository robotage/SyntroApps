# SyntroApps

SyntroApps contains all the SyntroNet apps in one repo.

Check out www.richards-tech.com for more details.

### Release history

#### November 7 2014 - 1.1.0

Updated RTIMULib to include MPU-9250 and SPI support

#### November 4 2014 - 1.0.1

Added SyntroGloveView.

### Dependencies

1. Qt4 or Qt5 development libraries and headers (Qt5 recommended if possible)
2. SyntroCore libraries and headers 
3. gstreamer sdk installed in standard location for GS apps


### Fetch and Build

####Linux

If SyntroCore has not been installed yet:

	mkdir ~/SyntroNet
	cd ~/SyntroNet
	git clone git://github.com/richards-tech/SyntroCore.git
	cd SyntroCore
	qmake
	make
	sudo make install

To compile the GS apps, install the gstreamer sdk at the standard location. See http://docs.gstreamer.com/display/GstSDK/Installing+the+SDK for Windows and Mac. For Ubuntu,

	sudo apt-get install libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev libgstreamer-plugins-bad0.10-dev	

	
Then:

	cd ~/SyntroNet
	git clone git://github.com/richards-tech/SyntroApps.git
	cd SyntroApps
	qmake SyntroApps.pro
	make
	sudo make install

To make the GS apps:

	qmake SyntroAppsGS.pro
	make
	sudo make install


Alternatively, each app can be individually compiled (better on lower power processors such as ARMs). For example, to compile SyntroView:

	cd ~/SyntroNet/SyntroApps/SyntroView
	qmake
	make
	sudo make install



