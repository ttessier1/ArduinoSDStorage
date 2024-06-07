# ArduinoSDStorage

Status: Alpha

The goal of this project is to provide a serial interface to upload and download files over serial from an sdcard using the arduino.
The basic environment uses the MKRWIFI1010 as the arduino with an sdcard inserted into a mkr ethernet shield

The serial interface uses decimal numbers as command followed by command arguments or carriage return line feed combination

The command list is similar to the following:

0) Ping - implemented - returns the word ping and is a trivial method to confirm message send and receive
1) List files - implemented ( current directory only )
2) Change Directory - not implemented
3) Delete File - not implemented
4) Create/Upload File - not implemented
5) Download File - implemented - not fully tested
6) Get File Size - implemented - returns a hexidecimal representation of the file size
7) Get Gile Hash - implemented - returns an sha256 hash

This basic command list allows for retreiving files, creating files and deleting files but it is a basic and naive implementation

The windows project is best suited for windows 11 and Visual Studio 2022 using the next gen BCrypt library for hash generation
