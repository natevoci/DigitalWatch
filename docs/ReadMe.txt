DigitalWatch Readme
```````````````````
Author: Nate
webpage: http://nate.dynalias.net/DigitalWatch/

Description
```````````
  DigitalWatch is a free open source DTV watching and recording program
  originally written for the VisionPlus DVB-T tuner, There were other
  options available, but I was unhappy with the lack of ability to
  customize many aspects of the tv watching experience, so i created
  DigitalWatch. Since then the code base has been re-written to support
  DVB-T tuners with BDA drivers.

  DigitalWatch is mostly oriented towards watching rather than recording.
  If you are more intereted in recording then try
  WebScheduler http://dvb-ws.sourceforge.net/

  DigitalWatch is not aimed at the novice user. Maybe it will be in the
  future, but for now i'm not going out of my way to make it easy for anyone
  who doesn't like to tinker. That said, if you have any questions then ask
  on the DVB-T Owners Forum and someone will probabaly help you out.


Getting started
```````````````
1. Set up channels
  DigitalWatch does not automatically scan for channels yet.
  To set up channels, open up the channels.ini file in the DigitalWatch folder
  and fill it in with the appropriate frequencies and PID's for your region.
  There are details in the channels.ini file on how to use ScanChannels.exe
  to scan for channels.
  Note: You can use a channels.ini file from a previous version of DigitalWatch
        the program number needs to be valid if you want the Now and Next
        function to work.

2. Load DigitalWatch

3. Press a number from 1 to 9 to select a network to start watching.

Keys / Mouse
````````````
  Left mouse click-and-drag to move the window.
  Left mouse double-click to toggle fullscreen.

  For key assignments see Keys.ini
  For function descriptions see docs\Functions.txt

Debugging
`````````
  In settings.ini there is an option to add the graph to ROT so you use
  Filter Graph to see exactly what filters are being used. People having
  trouble can use this to have a look and see what might be causing
  the trouble.



comments, suggestions, bug reports to nate@bigblue.net.au

