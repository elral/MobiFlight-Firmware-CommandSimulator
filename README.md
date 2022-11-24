# Sending a txt file to a COM port

This little program sends a text file to a COM port.
The program must be started from the commandline (or setup a batch file), COM port, baud rate, device and file name must following be parameters.
The intend is to simulate commands from the MobiFlight Connector to an attached board without starting the flight simulator.


## Starting the command simulator

Usage: simulate.exe "serialport" "device" "baud" "txt file"

The "device" could be 0 for a Mega or Uno, or it could be 1 for the ProMicro.

The "serialport" must be defined without COM.

The "baud" must be 115200 for all Mobiflight boards.

If the COM port is available, the commands will be printed to the COM port and theterminal.
Otherwise only to the terminal.

As an example for ProMicro:
* simulateUI.exe 3 115200 1 MF_Test.prn

## Format of the textfile

The format of the textfile is like the existing commands for each output device.
Additionally a delay for each command must be defined and terminated with ":".
The next command will be send after this delay.
So the format must be:

   3,        0,   500;   2000:

   3,        0,   200;   2000:

   3 = Stepper
   0 = 0th Stepper
 500 = set position to 500
2000 = delay of 2000ms

Every other command to output devices with the required parameter can be used.
But always the delay time has to added.

The text file can be easily set up with Excel (see demo excel file).
After setting up the file export it as .prn file.
Just be careful that the columns are not too small. In this case some characters will not be exported.

Another simple way is to mark all cells and then copy and paste them into a textfile (e.g. setup with the Windows Editor).
The format should be also like above, the spaces will be filtered out.