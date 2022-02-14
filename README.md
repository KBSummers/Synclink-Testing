# Synclink Testing


**Meant to be built and ran on a Linux environment** 

In order to build the test provided here, first you must clone this repository to your local  machines' file system:
```bash
$ git clone https://github.com/KBSummers/Synclink-Testing.git 
```
Then, to build all tests and utilities:
```bash
make
```
Change into the testloop directory:
```bash
cd src/loopback/
```
We can now run the loopback test from here, with either:
- No arguments (ran at default rate of 2kbps)
- One argument (rate of argument in bps)
``` bash
# 2kbps default
./testloop

# specified rate in bps
./testloop [date_rate]
```
There is a strong possibility of naming conflicts for the device (The linux kernel will not pick it up as the Microgate default naming convention "ttySLG")... Not to worry this can be solved with a udev rule. 

To rebuild the application, first run:
```bash
make clean
```
Then you may run `make` again to rebuild.
