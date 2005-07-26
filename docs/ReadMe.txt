DigitalWatch Readme
```````````````````
Author: Nate
webpage: http://nate.dynalias.net/

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
1. Load DigitalWatch

2. Use the Arrow keys and Enter and Esc to navigate the menu's
   If you loose the menu's then F1 and F2 will bring them up again.
   Basically just keep pressing enter until you get the to Add Network menu.

3. Select the frequency you want to add and press enter.

4. Wait until the number in the bottom right hand corner changes to the network name
   Or, if the scanning text disappears then it has timed out.

5. Repeat steps 3 and 4 for each network

6. Press Esc to get out of the Add Network menu.

7. When no menu's are visible use the Enter key to bring up the network selection list
   Use the right and left arrows to show or hide the services for each network
   Press enter to change to the selected network or service.
   

Keys / Mouse
````````````
  For key assignments see the Keys.xml and BDA_DVB-T\Keys.xml files.
  For function descriptions see docs\Functions.txt (not updated yet)

  Left mouse click-and-drag to move the window.
  Left mouse double-click to toggle fullscreen.
  Left mouse click-and-drag the border to resize.
  Shift Left mouse click-and-drag the border to change the aspect ratio.

Debugging
`````````
  In Settings.xml there is an option to add the graph to ROT so you use
  Filter Graph (graphedt.exe) to see exactly what filters are being used.
  People having trouble can use this to have a look and see what might
  be causing the trouble.


comments, suggestions, bug reports to an appropriate thread on the forum
http://forums.dvbowners.com/

DigitalWatch Development thread
http://forums.dvbowners.com/index.php?showforum=25
