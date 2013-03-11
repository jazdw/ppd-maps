PPD Maps
========

PPD Maps is open source (GPLv3) software for detecting maps in Siemens ECU dumps. Supported ECUs are PPD, SID206 & SID803A. There is partial support for SID803 and barely working support for SID201

The software will identify the corresponding axes for each map and also give a list of DTC/P-codes. It will also find single byte switches to turn off DTCs

The file must be a full read of the ECU i.e. BDM. A partial read using a tool such as MPPS or similar will not work.

Export to XDF format is possible, please use the latest TunerPro software to open it. Maps can be imported into WinOLS via the A2L (ASAP2) export function.

For more information please see http://jazdw.net/ppd-maps/

Compiling
==========
Compile with Qt 4.7+ (Project file included)
Link against QwtPlot3D - http://qwtplot3d.sourceforge.net/
