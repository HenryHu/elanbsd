#ifndef _ELANBSD_DISPLAY_H_
#define _ELANBSD_DISPLAY_H_

#include "config.h"

#ifdef USE_XTEST
#include <X11/extensions/XTest.h>
#endif

#include <stdio.h>
#include <math.h>
#include "elanbsd.h"

class XDisplay {
	int dpy_x, dpy_y;
#ifdef USE_XTEST
	Display *display;
	int screen_number;
#endif
	public:
	XDisplay() {
		FILE *f = popen("xdotool getdisplaygeometry", "r");
		fscanf(f, "%d %d", &dpy_x, &dpy_y);
		pclose(f);
#ifdef USE_XTEST
		display = XOpenDisplay(NULL);
		if (!display)
			errexit("fail to open display");
		int event_base, err_base, major, minor;
		if (!XTestQueryExtension(display, &event_base, &err_base, &major, &minor))
			errexit("No XTEST extension found");
		printf("using XTEST %d.%d\n", major, minor);
		screen_number = -1;
#endif
	}

	~XDisplay() {
#ifdef USE_XTEST
		XCloseDisplay(display);
#endif
	}

	void get_mouse_pos(int &x, int &y) {
		FILE *f = popen("xdotool getmouselocation", "r");
		fscanf(f, "x:%d y:%d", &x, &y);
		pclose(f);
	}

	void move(int x, int y) {
		printf("dpy:move %d,%d\n", x, y);
#ifdef USE_XTEST
		Bool succ = XTestFakeMotionEvent(display, screen_number, x, y, CurrentTime);
		if (!succ)
			errexit("fail to fake motion event");
		XFlush(display);
#else
		char buf[1024];
		snprintf(buf, sizeof(buf), "xdotool mousemove -- %d %d", x, y);
		system(buf);
#endif
	}

	void move_rel(double dx, double dy, int x_max, int y_max) {
		int rx = round(dx * dpy_x * MOVE_X_SCALE / x_max);
		int ry = round(-dy * dpy_y * MOVE_Y_SCALE / y_max);
		printf("dpy:move_rel %d,%d\n", rx, ry);
		if (rx == 0 && ry == 0) return;
#ifdef USE_XTEST
		Bool succ = XTestFakeRelativeMotionEvent(display, rx, ry, CurrentTime);
		if (!succ)
			errexit("fail to fake relative motion event");
		XFlush(display);
#else
		char buf[1024];
		snprintf(buf, sizeof(buf), "xdotool mousemove_relative -- %d %d", rx, ry);
		system(buf);
#endif
	}
	void click(int btn) {
		printf("dpy:click %d\n", btn + 1);
#ifdef USE_XTEST
		XTestFakeButtonEvent(display, btn + 1, True, CurrentTime);
		XTestFakeButtonEvent(display, btn + 1, False, CurrentTime);
		XFlush(display);
#else
		char buf[1024];
		snprintf(buf, sizeof(buf), "xdotool click %d", btn + 1);
		system(buf);
#endif
	}
	void press(bool down, int id) {
		printf("dpy:press %d,%s\n", id, down ? "down" : "up");
#ifdef USE_XTEST
		XTestFakeButtonEvent(display, id + 1, down ? True : False, CurrentTime);
		XFlush(display);
#else
		char buf[1024];
		snprintf(buf, sizeof(buf), "xdotool mouse%s %d", down ? "down" : "up", id + 1);
		system(buf);
#endif
	}
	int get_keycode(const std::string& str) {
#ifdef USE_XTEST
		return XKeysymToKeycode(display, XStringToKeysym(str.c_str()));
#else
		char buf[1024];
		snprintf(buf, sizeof(buf), "xmodmap -pke | grep ' %s ' | cut -w -f 2-2", str.c_str());
		int code;
		FILE *f = popen(buf, "r");
		if (fscanf(f, "%d", &code) != 1) errexit("fail to obtain keycode");
		pclose(f);
		return code;
#endif
	}
	void key_down(int keycode) {
		key(true, keycode);
	}
	void key_up(int keycode) {
		key(false, keycode);
	}
	void key(bool down, int keycode) {
#ifdef USE_XTEST
		XTestFakeKeyEvent(display, keycode, down ? True : False, CurrentTime);
		XFlush(display);
#else
		char buf[1024];
		snprintf(buf, sizeof(buf), "xdotool key%s %d", down ? "down" : "up", keycode);
		system(buf);
#endif
	}

};


#endif // _ELANBSD_DISPLAY_H_
