#ifndef _ELANBSD_FINGER_H_
#define _ELANBSD_FINGER_H_

#include "xdisplay.h"
#include "elanbsd.h"

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
	bool tap_for_drag;
	bool test_tap_drag;
	XDisplay *dpy;
	int history_x[HISTORY_LEN], history_y[HISTORY_LEN];
	int history_pos, history_cnt;
public:
	Finger() {
		touched = false;
		is_tap = false;
		pos_saved = false;
		tap_for_drag = false;
		test_tap_drag = false;
		history_pos = 0;
		history_cnt = 0;
	}
	inline double get_px_dist(double phy_dist) {
		return phy_dist * x_max / width;
	}
	// physical distance (mm)
	inline double get_phy_dist(double dist) {
		return fabs(dist) * width / x_max;
	}
	void set_dpy(XDisplay *newdpy) { dpy = newdpy; }
	int get_x() { return x; }
	int get_y() { return y; }
	void set_id(int newid) {
		id = newid;
	}
	bool in_vscroll() {
		if (x > x_max * VSCROLL_RANGE) {
			return true;
		} else {
			return false;
		}
	}
	int accel(int arg) {
		return (ACCEL_SCALE + abs(arg)) * arg / (ACCEL_SCALE + ACCEL_ACCELERATION);
	}
	void add_history(int dx, int dy) {
		static long last_tick = 0;
		struct timeval tv;
		dx = accel(dx);
		dy = accel(dy);

		gettimeofday(&tv, NULL);
		printf("%ld(%ld) new history: %d %d\n", tv.tv_usec,
				(tv.tv_usec - last_tick + 1000000) % 1000000, dx, dy);
		last_tick = tv.tv_usec;
		history_x[history_pos] = dx;
		history_y[history_pos] = dy;
		history_pos = (history_pos + 1) % HISTORY_LEN;
		if (history_cnt < HISTORY_LEN)
			history_cnt++;
	}
	void get_average(double &avg_x, double &avg_y) {
		double sx = 0, sy = 0;
		int weight = 0;
		int window_size = AVG_WINDOW_SIZE;
		if (history_cnt < window_size) window_size = history_cnt;
		int avg_pos = (history_pos - 1 + HISTORY_LEN) % HISTORY_LEN;
		for (int i=0; i<window_size; i++) {
			sx += history_x[avg_pos] * AVG_WINDOW[i];
			sy += history_y[avg_pos] * AVG_WINDOW[i];
			weight += AVG_WINDOW[i];
			if (avg_pos == 0)
				avg_pos = HISTORY_LEN - 1;
			else
				avg_pos -= 1;
		}
		avg_x = sx / weight;
		avg_y = sy / weight;
		printf("len: %d avg_x: %f avg_y: %f pos: %d\n", history_cnt, avg_x, avg_y,
				history_pos);
	}
	void new_pos(bool domove) {
		printf("%d: x moved: %d y moved: %d\n", id, x-x_start, y-y_start);
		if (is_tap)
			// if we moved too far from the starting point, then it is not a tap
			if (abs(x-x_start) > x_max * TAP_X_LIMIT / width || abs(y-y_start) > y_max * TAP_Y_LIMIT / width) {
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
				if (dy > get_px_dist(VSCROLL_LIMIT)) {
					dpy->click(3);
					vs_last_x = x;
					vs_last_y = y;
				} else if (dy < -get_px_dist(VSCROLL_LIMIT)) {
					dpy->click(4);
					vs_last_x = x;
					vs_last_y = y;
				}
			} else {
				int dx = x - lx;
				int dy = y - ly;
				add_history(dx, dy);
				double avg_x, avg_y;
				get_average(avg_x, avg_y);
				printf("avg. movement: %f %f\n", avg_x, avg_y);
				if (get_phy_dist(avg_x) >= MOVE_DX_LIMIT
						|| get_phy_dist(avg_y) >= MOVE_DY_LIMIT) {
					dpy->move_rel(avg_x, avg_y, x_max, y_max);
				} else {
					printf("movement filtered: dx=%f dy=%f limit=%f,%f\n",
							get_phy_dist(dx), get_phy_dist(dy),
							MOVE_DX_LIMIT, MOVE_DY_LIMIT);
				}
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

			if (test_tap_drag) {
				tap_for_drag = true;
				printf("tap: start drag!\n");
				dpy->press(true, 0);
			}
		}
		touched = true;
	}
	int get_btn_id() {
		int btn;
		// TODO: configurable area
		if (y < y_max * BTN_RANGE_Y) {
			if (x < x_max * BTN_0_RANGE_X) {
				btn = 0;
			} else if (x > x_max * BTN_2_RANGE_X) {
				btn = 2;
			} else {
				btn = 1;
			}
		} else {
			btn = 0;
		}
		return btn;
	}
	struct click_timeout_args {
		Finger *finger;
		int btn_id, start_x, start_y;
	};
	static void click_timeout(void *arg) {
		struct click_timeout_args *args = (struct click_timeout_args *)arg;
		if (args->finger->tap_for_drag) {
			delete args;
			return;
		}
		printf("tap: a real tap\n");
		args->finger->dpy->move(args->start_x, args->start_y);
		args->finger->dpy->click(args->btn_id);
		args->finger->test_tap_drag = false;
		delete args;
	}
	void release(bool clicked, int &cnt, int max_cnt) {
		int lcnt = cnt;
		if (touched) {
			printf("%d release\n", id);
			cnt --;
			if (test_tap_drag && tap_for_drag) {
				printf("tap: stop drag!\n");
				dpy->press(false, 0);
			}
		}
		touched = false;
		pos_saved = false;
		test_tap_drag = false;
		history_cnt = 0;

		// if we moved 
		if (is_tap && !clicked && !tap_for_drag && !DISABLE_TAP) {
			struct timeval up_time;
			gettimeofday(&up_time, NULL);
			int delta_time = (up_time.tv_sec - down_time.tv_sec) * 1000 + (up_time.tv_usec - down_time.tv_usec) / 1000;
			if (delta_time > TAP_TIMEOUT) {
				printf("tap timeout: %d\n", delta_time);
			} else {
				printf("tap! time: %d\n", delta_time);
				if (max_cnt == 1) {
					// set a timeout before clicking
					struct click_timeout_args *args = new struct click_timeout_args();
					tap_for_drag = false;
					test_tap_drag = true;
					printf("tap delayed: test for drag\n");
					args->finger = this;
					args->btn_id = get_btn_id();
					args->start_x = mouse_start_x;
					args->start_y = mouse_start_y;
					create_timeout(click_timeout, args, TAP_TO_DRAG_TIMEOUT);
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
		tap_for_drag = false;
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


#endif // _ELANBSD_FINGER_H_
