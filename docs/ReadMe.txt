DigitalWatch Readme
```````````````````
Author: Nate
email: nate@bigblue.net.au
webpage: http://home.bigblue.net.au/mulin77/DigitalWatch/

Description
```````````
  DigitalWatch is a free open source DTV watching and recording program
  to for the VisionPlus DVB-T. There are other options available, such as
  WinDTV/VisionDTV and Digibox/DigiVision, but I was unhappy with the lack
  of ability to customize many aspects of the tv watching experience, so i
  created DigitalWatch

  DigitalWatch is mostly oriented towards watching rather than recording
  although it does have recording capabilities. If you are more intereted
  in recording then try WebScheduler http://www.digtv.ws/html/dvb/index.php

  There is also a modified CaptureEngine.dll for WebScheduler that will allow
  WebScheduler to use DigitalWatch to record if DigitalWatch is running.
  It will also automatically close VisionDTV and DigiVision if the are
  running before it tries to record.
  http://home.bigblue.net.au/mulin77/DigitalWatch/#WSM

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

