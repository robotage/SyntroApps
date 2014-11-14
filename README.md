# SyntroApps

SyntroApps contains all the SyntroNet apps in one repo.

Check out www.richards-tech.com for more details.

### Release history

#### November 14 2014 - 1.2.2

Updated RTIMULib to latest version (4.1.0)

#### November 13 2014 - 1.2.1

Improved SyntroViewGS startup on Windows

#### November 13 2014 - 1.2.0

Upgraded SyntroViewGS to gstreamer 1.0 on Linux (still 0.10 on Windows).

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

	mkdir ~/richards-tech
	cd ~/richards-tech
	git clone git://github.com/richards-tech/SyntroCore.git
	cd SyntroCore
	qmake
	make
	sudo make install

To compile the GS apps, install the gstreamer sdk at the standard location. See http://docs.gstreamer.com/display/GstSDK/Installing+the+SDK for Windows and Mac. For Ubuntu,

	sudo apt-get install libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev libgstreamer-plugins-bad0.10-dev	
	sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-libav	

	
Then:

	cd ~/richards-tech
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

	cd ~/richards-tech/SyntroApps/SyntroView
	qmake
	make
	sudo make install



