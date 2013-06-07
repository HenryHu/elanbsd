#ifndef _ELANBSD_FINGER_H_
#define _ELANBSD_FINGER_H_

#include "xdisplay.h"

#include <stdio.h>
#include <math.h>
#include <sys/time.h>

class Finger {
	int x, y;
	int lx, ly;
	int x_start, y_start;
	int vs_last_x, vs_last_y;
	int saved_x, saved_y;
	int mouse_start_x, mouse_start_y;
	int pres, traces;
	int x_max, y_max, width;
	struct timeval down_time;
	int id;
	bool is_tap, is_vscroll;
	bool touched;
	bool pos_saved;
	XDisplay *dpy;
public:
	Finger() {
		touched = false;
		is_tap = false;
		pos_saved = false;
	}
	void set_dpy(XDisplay *newdpy) { dpy = newdpy; }
	int get_x() { return x; }
	int get_y() { return y; }
	void set_id(int newid) {
		id = newid;
	}
	bool in_vscroll() {
		if (x > x_max * 9 / 10) {
			return true;
		} else {
			return false;
		}
	}
	void new_pos(bool domove) {
		printf("%d: x moved: %d y moved: %d\n", id, x-x_start, y-y_start);
		if (is_tap)
			// if we moved too far from the starting point, then it is not a tap
			if (abs(x-x_start) > x_max * 2 / width || abs(y-y_start) > y_max * 2 / width) {
				printf("not tap\n");
				is_tap = false;
			}
		// we assume that new_pos() is called before touch(), so
		// touched is false for the first event
		if (touched && domove) {
			// for vscroll, we send an event after certain amount of movement
			if (is_vscroll) {
				int dx = x - vs_last_x;
				int dy = y - vs_last_y;
				// TODO: configurable distance
				if (dy > y_max / 50) {
					dpy->click(3);
					vs_last_x = x;
					vs_last_y = y;
				} else if (dy < -y_max / 50) {
					dpy->click(4);
					vs_last_x = x;
					vs_last_y = y;
				}
			} else {
				dpy->move_rel(x - lx, y - ly, x_max, y_max);
			}
		}
		lx = x;
		ly = y;

	}
	void set_pos(int newx, int newy, bool domove) {
		x = newx;
		y = newy;
		new_pos(domove);
	}
	void add_delta(int dx, int dy, bool domove) {
		x += dx;
		y += dy;
		new_pos(domove);
	}
	void touch() {
		if (!touched) {
			// record everything
			// we assume new_pos() is called before touch()
			// so these info corresponds to the new touch
			x_start = x;
			y_start = y;
			vs_last_x = x;
			vs_last_y = y;
			// we assume it is tap at first
			is_tap = true;
			is_vscroll = in_vscroll();
			// save mouse position
			// if it is tap, restore position
			dpy->get_mouse_pos(mouse_start_x, mouse_start_y);
			gettimeofday(&down_time, NULL);
		}
		touched = true;
	}
	int get_btn_id() {
		int btn;
		// TODO: configurable area
		if (y < y_max / 5) {
			if (x < x_max * 2 / 5) {
				btn = 0;
			} else if (x > x_max * 3 / 5) {
				btn = 2;
			} else {
				btn = 1;
			}
		} else {
			btn = 0;
		}
		return btn;
	}
	void release(bool clicked, int &cnt, int max_cnt) {
		int lcnt = cnt;
		if (touched) {
			printf("%d release\n", id);
			cnt --;
		}
		touched = false;

		// if we moved 
		if (is_tap && !clicked) {
			struct timeval up_time;
			gettimeofday(&up_time, NULL);
			if (up_time.tv_sec > down_time.tv_sec || up_time.tv_usec > down_time.tv_usec + 1000 * 100) {
				printf("tap timeout\n");
			} else {
				printf("tap!\n");
				if (max_cnt == 1) {
					dpy->move(mouse_start_x, mouse_start_y);
					dpy->click(get_btn_id());
					// You cannot simply say "cnt == 0"
					// because multiple fingers may be released in the same event
					// (Although this should not happen)
				} else if (lcnt == 1 && cnt == 0) {
					if (max_cnt == 2) {
						dpy->click(2);
					} else if (max_cnt == 3) {
						dpy->click(1);
					}
				}
			}
		}
		is_tap = false;
	}
	void set_info(int newpres, int newtraces) {
		pres = newpres;
		traces = newtraces;
	}
	void set_range(int _x_max, int _y_max, int _width) {
		x_max = _x_max;
		y_max = _y_max;
		width = _width;
	}
	bool is_touched() { return touched; }
	double dist(const Finger &other) {
		return sqrt((other.x - x) * (other.x - x) + (other.y - y) * (other.y - y));
	}
	bool get_delta(int &dx, int &dy) {
		if (pos_saved) {
			dx = x - saved_x;
			dy = y - saved_y;
			return true;
		} else return false;
	}
	bool get_delta_dist(double &d) {
		if (pos_saved) {
			double dx = x - saved_x;
			double dy = y - saved_y;
			d = sqrt(dx*dx + dy*dy);
			return true;
		} else return false;
	}
	void save_pos() {
		saved_x = x;
		saved_y = y;
		pos_saved = true;
	}

};


#endif // _ELANBSD_MOUSE_H_
