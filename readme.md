XboxOverclock
============

XboxOverclock will allow you to overclock the FSB and NVCLK via software. It uses infromation gained from [XBOverclock](https://github.com/WulfyStylez/XBOverclock) by WulfyStylez.

Building
---------

See the requirements for [nxdk](https://github.com/XboxDev/nxdk/wiki/Install-the-Prerequisites).


```sh
git clone https://github.com/GXTX/XboxOverclock
cd XboxOverclock
eval $(../nxdk/bin/activate -s) make -f Makefile.nxdk
```

Then you'll have the xbe in the `bin` folder.

Running
---------

Once you boot into the xbe you'll see a prompt. Use the left and right D-Pad to change the wanted FSB frequency, use the up and down D-Pad to change NVCLK frequency.
