elanbsd
=======

Elan Touchpad driver for BSD

Supported Hardware
------------------

Probably works for most laptops with elantech touchpads, but it is known to work for the Asus Zenbook UX31e (the one with Elantech touchpad).

Installation
------------

Make sure you have the required dependencies:

* xdotool : `pkg install xdotool`

Then run `make` and copy the `elanbsd` binary to some `bin/` folder.

You can then add this line to your `.xinitrc`:

* `/path/to/elanbsd &`

Troubleshooting
---------------

Before installing into `.xinitrc`, you should test that it works correctly with your touchpad. Run `make test` and move the mouse, see if it logs any error and if the pointer actually moves.

If you get `fail to open psm0`, then it is possible that the device is already loaded by some other driver.

One thing that worked:

* make sure moused is not running (`service -e` will give you the list of enabled daemons).
* remove the device from HAL (`sudo hal-device -r psm_0`).
* configure X so that it ignores this device (in /etc/X11/xorg.conf, add this section)
    
    Section "InputClass"
            Identifier "No Mouse"
            MatchDevicePath "/dev/psm*"
            Option "Ignore" "yes"
    EndSection


Configuration
-------------

Some options may be configured in the file `config.h`, which is well commented. Options include the distance required for a pinch-to-zoom motion to trigger a zoom in or out, the area of the touchpad to use for vertical scrolling and various buttons, disabling the tap-to-click, and controlling the two-finger scroll direction.
