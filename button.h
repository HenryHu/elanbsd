#ifndef _ELANBSD_BUTTON_H_
#define _ELANBSD_BUTTON_H_

#include "xdisplay.h"

class Button {
	bool down;
	int id;
	XDisplay *dpy;
public:
	void set_dpy(XDisplay *newdpy) { dpy = newdpy; }
	void set_id(int newid) { id = newid; }
	void update(bool state) {
		if (down != state) {
			down = state;
			printf("button %d %s\n", id, down ? "down" : "up");
			dpy->press(down, id);
		}
	}
	bool is_down() { return down; }
};


#endif // _ELANBSD_BUTTON_H_
