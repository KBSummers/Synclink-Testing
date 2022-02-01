# Synclink Testing
**Meant to be built and ran on a Linux environment** 

In order to build the test provided here, first you must clone this repository to your local  machines' file system:
```bash
$ git clone https://github.com/KBSummers/Synclink-Testing.git 
```
Then simply build to the source code into an executable and run the test, with the following commands:
```bash
make
./test
```
The test will automatically look for a device titled: `TTYSLG0`, a naming convention used by Microgate for their serial devices. If it does not find this, it will parse for devices with the "USB"" title. This should find the synclink device... but if not this can be solved by suppliying the device name as a command line argument such as:
```bash
./test <device_name>
```
To rebuild the application, first run:
```bash
make clean
```
Then you may run `make` again to rebuild.
