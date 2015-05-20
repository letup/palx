### Q: What is this project for? ###
A: See [this topic](http://gamerhome.net/bbs/viewthread.php?tid=231593) for detail.

### Q: How to start playing? ###
A: It depends on progress. Anytime you can play in-place, and this method suits windows users best. Currently, using 'palx x' for write config file, and you can then modify the path in the config section, and etc, for more functions. In future a wrapper may be introduced to reduce the work of typical nix user.

### Q: Is there any schedule or plan on improvement? ###
A: Currently no scheduling. There are several TODO, but any of them seems to be less important than just complete the mainline trip. Currently they are: mouse support, win95/Sega Saturn version resource support, video/audio enhancement, UI rearchitecture.

### Q: How many platform has been actually supported currently? ###
A: In fact only two platforms have been fully tested: win32, and GNU/Linux. Myself can only testing on winxp sp2 & ubuntu gutsy gibbon, Whistler helps testing on Debian(v?) and kubuntu feisty fawn, Shenyanduxing helps testing on windows vista, and Shin Panda helps testing on windows 98SE. All combinations works: xp windowed/fullscreen, Linux console frame buffer(in this case you may not get a regular 320x200 viewport; your current framebuffer size instead of it and the unsupported stretch mode works)/x window/x winnow fullscreen, and vista windowed - Oh, no, vista fullscreen failed. And windowed/fullscreen switch only available on win32, now. Win32s platform seems not having the ability  of conversion to/from Unicode, so the text seems to be a confusion. About other platforms I've promised - DOS family, Mac OS X, anyone who has the environment can help us testing and fixing bugs, isn't it? :)