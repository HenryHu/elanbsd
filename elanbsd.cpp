#include <iostream>
#include <sys/mouse.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
using namespace std;

/* aux device commands (sent to KBD_DATA_PORT) */
#define PSMC_RESET_DEV	     	0x00ff
#define PSMC_ENABLE_DEV      	0x00f4
#define PSMC_DISABLE_DEV     	0x00f5
#define PSMC_SET_DEFAULTS	0x00f6
#define PSMC_SEND_DEV_ID     	0x00f2
#define PSMC_SEND_DEV_STATUS 	0x00e9
#define PSMC_SEND_DEV_DATA	0x00eb
#define PSMC_SET_SCALING11	0x00e6
#define PSMC_SET_SCALING21	0x00e7
#define PSMC_SET_RESOLUTION	0x00e8
#define PSMC_SET_STREAM_MODE	0x00ea
#define PSMC_SET_REMOTE_MODE	0x00f0
#define PSMC_SET_SAMPLING_RATE	0x00f3

const unsigned char ETP_FW_ID_QUERY = 0x00;
const unsigned char ETP_FW_VERSION_QUERY = 0x01;
const unsigned char ETP_CAPABILITIES_QUERY = 0x02;
const unsigned char ETP_SAMPLE_QUERY = 0x03;
const unsigned char ETP_RESOLUTION_QUERY = 0x04;

const unsigned char ETP_PS2_CUSTOM_COMMAND = 0xf8;

const unsigned char ETP_REGISTER_READWRITE = 0x00;

const int ETP_MAX_FINGERS = 5;
const int ETP_MAX_BUTTONS = 20;

const int HEAD = 1;
const int MOTION = 2;
const int STATUS = 3;
const int UNKNOWN = 0;

void errexit(const string& err) {
	cerr << err << endl;
	exit(errno);
}

class Mouse;

class Display {
	int dpy_x, dpy_y;
public:
	Display() {
		FILE *f = popen("xdotool getdisplaygeometry", "r");
		fscanf(f, "%d %d", &dpy_x, &dpy_y);
		fclose(f);
	}
	void get_mouse_pos(int &x, int &y) {
		FILE *f = popen("xdotool getmouselocation", "r");
		fscanf(f, "x:%d y:%d", &x, &y);
		fclose(f);
	}
	void move(int x, int y) {
		char buf[1024];
		snprintf(buf, sizeof(buf), "xdotool mousemove -- %d %d", x, y);
		system(buf);
	}
	void move_rel(int dx, int dy, int x_max, int y_max) {
		int rx = dx * dpy_x / x_max;
		int ry = -dy * dpy_y / y_max;
		printf("move %d,%d\n", rx, ry);
		char buf[1024];
		snprintf(buf, sizeof(buf), "xdotool mousemove_relative -- %d %d", rx, ry);
		system(buf);
	}
	void click(int btn) {
		char buf[1024];
		snprintf(buf, sizeof(buf), "xdotool click %d", btn + 1);
		system(buf);
	}
};

class Button {
	bool down;
	int id;
public:
	void set_id(int newid) { id = newid; }
	void update(bool state) {
		if (down != state) {
			down = state;
			printf("button %d %s\n", id, down ? "down" : "up");
			char buf[1024];
			snprintf(buf, sizeof(buf), "xdotool mouse%s %d", down ? "down" : "up", id + 1);
			system(buf);
		}
	}
};

class Finger {
	int x, y;
	int lx, ly;
	int x_start, y_start;
	int vs_last_x, vs_last_y;
	int mouse_start_x, mouse_start_y;
	int pres, traces;
	int x_max, y_max, width;
	struct timeval down_time;
	int id;
	bool is_tap, is_vscroll;
	bool touched;
public:
	Display *dpy;
	Finger() {
		touched = false;
		is_tap = false;
	}
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
		printf("x moved: %d y moved: %d\n", x-x_start, y-y_start);
		if (is_tap)
			if (abs(x-x_start) > x_max * 2 / width || abs(y-y_start) > y_max * 2 / width) {
				printf("not tap\n");
				is_tap = false;
			}
		if (touched && domove) {
			if (is_vscroll) {
				int dx = x - vs_last_x;
				int dy = y - vs_last_y;
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
			x_start = x;
			y_start = y;
			vs_last_x = x;
			vs_last_y = y;
			is_tap = true;
			is_vscroll = in_vscroll();
			dpy->get_mouse_pos(mouse_start_x, mouse_start_y);
			gettimeofday(&down_time, NULL);
		}
		touched = true;
	}
	int get_btn_id() {
		int btn;
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
	void release(bool clicked) {
		if (touched)
			printf("%d release\n", id);
		touched = false;

		printf("%d tap: %d\n", id, is_tap);
		if (is_tap && !clicked) {
			struct timeval up_time;
			gettimeofday(&up_time, NULL);
			if (up_time.tv_sec > down_time.tv_sec || up_time.tv_usec > down_time.tv_usec + 1000 * 100) {
				printf("tap timeout\n");
			} else {
				printf("tap!\n");
				dpy->move(mouse_start_x, mouse_start_y);
				dpy->click(get_btn_id());
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
};

class Mouse {
	int ver_major, ver_minor, ver_step;
	int x_max, y_max, width;
	int hw_ver;
	int cnt;
	int fd;
	bool clicked;
	unsigned char cap[3];
	Finger fingers[ETP_MAX_FINGERS];
	Button btns[ETP_MAX_BUTTONS];

	void check(int ret, const string& err) {
		if (ret < 0) errexit(err);
	}
	Display dpy;

public:
	Mouse() {
		for (int i=0; i<ETP_MAX_FINGERS; i++) {
			fingers[i].set_id(i);
			fingers[i].dpy = &dpy;
		}
		for (int i=0; i<sizeof(btns) / sizeof(Button); i++)
			btns[i].set_id(i);

	}
	void open_dev() {
		mousemode_t info;
		fd = open("/dev/bpsm0", O_RDWR);
		if (fd < 0) errexit("fail to open psm0");

		int level = 2;

		/*	check(ioctl(fd, MOUSE_GETMODE, &info), "fail to get mode");

			info.level = 2;
			check(ioctl(fd, MOUSE_SETMODE, &info), "fail to set mode");*/

		check(ioctl(fd, MOUSE_SETLEVEL, &level), "fail to set level");
		printf("open_dev() OK\n");
	}
	int read_data(unsigned char *buf, int len) {
		int count = 0;
		for (int i=0; i<len; i++) {
			int ret = read(fd, &buf[i], 1);
			if (ret != 1)
				return count;
			count++;
		}
		return count;
	}
	int write_data(unsigned char *buf, int len) {
		return write(fd, buf, len);
	}
	int send_cmd(unsigned char cmd, unsigned char *buf = NULL, int paramlen = 0, int retlen = 0) {
		int ret = write_data(&cmd, 1);
		if (ret < 0)
			errexit("fail to write cmd");
		if (paramlen > 0) {
			ret = write_data(buf, paramlen);
			if (ret != paramlen)
				errexit("fail to write cmd args");
		}
		unsigned char cmd_ret;
		ret = read_data(&cmd_ret, 1);
		if (ret!=1)
			errexit("fail to read cmd ret");
		if (retlen > 0) {
			ret = read_data(buf, retlen);
			if (ret < retlen) {
//				printf("ret: %d\n", ret);
				errexit("fail to read cmd result");
			}
		}
		return cmd_ret;
	}
	bool detect() {
		send_cmd(PSMC_SET_DEFAULTS);
		send_cmd(PSMC_DISABLE_DEV);
		send_cmd(PSMC_SET_SCALING11);
		send_cmd(PSMC_SET_SCALING11);
		send_cmd(PSMC_SET_SCALING11);

		unsigned char buf[100];
		send_cmd(PSMC_SEND_DEV_STATUS, buf, 0, 3);
//		dump_buf(buf, 3);
		if (buf[0] == 0x3c && buf[1] == 0x03 && buf[2] == 0x00) {
			printf("Elan Touchpad detected.\n");

			get_ver();
			return true;
		}
		return false;
	}

	void get_ver() {
		unsigned char param[100];
		param[0] = param[1] = param[2] = 0;
		send_adv_cmd(ETP_FW_VERSION_QUERY, param);
		printf("ver: %d.%d.%d\n", param[0], param[1], param[2]);
		ver_major = param[0];
		ver_minor = param[1];
		ver_step = param[2];
	}

	void dump_buf(char *buf, int len) {
		printf("len: %d\n", len);
		for (int i=0; i<len; i++) {
			printf("%02x ", (unsigned)buf[i]);
		}
		printf("\n");
	}
	void dump_loop() {
		while (true) {
			unsigned char buf[100];
			int ret = read_data(buf, 1);
			//		if (errno == 35) continue;
			if (ret != 1) errexit("fail to read data");
			printf("%02x ", buf[0]);
			fflush(stdout);

			/*		mousedata_t data;
			//		data.len = sizeof(data.buf) / sizeof(data.buf[0]);
			data.len = 1;
			check(ioctl(fd, MOUSE_READDATA, &data), "fail to read data");*/
		}
	}
	int send_adv_cmd(unsigned char cmd, unsigned char *param) {
		send_cmd(PSMC_SET_SCALING11);
		for (int i=0; i<4; i++) {
			unsigned char part = (cmd >> ((3-i)*2)) & 3;
			send_cmd(PSMC_SET_RESOLUTION, &part, 1, 1);
		}

		return send_cmd(PSMC_SEND_DEV_STATUS, param, 0, 3);
	}
	int send_adv_cmd_new(unsigned char cmd, unsigned char *param) {
		send_cmd(ETP_PS2_CUSTOM_COMMAND);
		send_cmd(cmd);
		return send_cmd(PSMC_SEND_DEV_STATUS, param, 0, 3);
	}
	void get_hwver() {
		switch(ver_major & 0x0f) {
			case 6:
				hw_ver = 4;
				break;
		}
		printf("hw ver: %d\n", hw_ver);
	}
	void get_cap() {
		send_adv_cmd(ETP_CAPABILITIES_QUERY, cap);
		printf("cap: %02x %02x %02x\n", cap[0], cap[1], cap[2]);
	}
	void write_reg(int reg, unsigned char val) {
		switch(hw_ver) {
			case 4:
				send_cmd(ETP_PS2_CUSTOM_COMMAND);
				send_cmd(ETP_REGISTER_READWRITE);
				send_cmd(ETP_PS2_CUSTOM_COMMAND);
				send_cmd((unsigned char)reg);
				send_cmd(ETP_PS2_CUSTOM_COMMAND);
				send_cmd(ETP_REGISTER_READWRITE);
				send_cmd(ETP_PS2_CUSTOM_COMMAND);
				send_cmd(val);
				send_cmd(PSMC_SET_SCALING11);
				break;
		}
	}
	void set_absolute() {
		switch (hw_ver) {
			case 4:
				write_reg(7, 1);
				break;
		}
	}
	void get_range() {
		unsigned char buf[3];
		switch (hw_ver) {
			case 4:
				send_adv_cmd(ETP_FW_ID_QUERY, buf);
				x_max = (0x0f & buf[0]) << 8 | buf[1];
				y_max = (0xf0 & buf[0]) << 4 | buf[2];
				width = x_max / (cap[1] - 1);
				break;
		}
		for (int i=0; i<ETP_MAX_FINGERS; i++)
			fingers[i].set_range(x_max, y_max, width);
		printf("x_max: %d y_max: %d width: %d\n", x_max, y_max, width);
	}
	void enable() {
		send_cmd(PSMC_ENABLE_DEV);
	}
	bool init() {
		get_hwver();
		get_cap();
		get_range();
		set_absolute();
		enable();
		printf("init() OK\n");
		return true;
	}

	void parse_pkt(unsigned char *buf, int len) {
		int type = UNKNOWN;
		if ((buf[0] & 0x0c) == 0x04) {
			switch (buf[3] & 0x1f) {
				case 0x11:
					type = HEAD;
					break;
				case 0x12:
					type = MOTION;
					break;
				case 0x10:
					type = STATUS;
					break;
			}
		}

		bool btn_down = buf[0] & 0x01;
		if (cnt == 1) {
			int btn;
			for (int i=0; i<ETP_MAX_FINGERS; i++) {
				if (fingers[i].is_touched()) {
					btn = fingers[i].get_btn_id();
					break;
				}
			}
			if (btn_down)
				clicked = true;
			btns[btn].update(btn_down);
		}

		switch(type) {
			case HEAD:
				{
					int id = ((buf[3] & 0xe0) >> 5) - 1;
					int x = (buf[1] & 0x0f) << 8 | buf[2];
					int y = (buf[4] & 0x0f) << 8 | buf[5];
					int pres = (buf[1] & 0xf0) | ((buf[4] & 0xf0) >> 4);
					int traces = (buf[0] & 0xf0) >> 4;
					printf("id: %d x: %d y: %d pres: %d width: %d\n", id, x, y, pres, traces);
					fingers[id].set_pos(x, y, cnt == 1);
					fingers[id].touch();
					fingers[id].set_info(pres, traces);

					break;
				}
			case MOTION:
				{
					int id = ((buf[0] & 0xe0) >> 5) - 1;
					int sid = ((buf[3] & 0xe0) >> 5) - 1;
					int weight = (buf[0] & 0x10) ? 5 : 1;
					int dx1 = (char)buf[1] * weight;
					int dy1 = (char)buf[2] * weight;
					int dx2 = (char)buf[4] * weight;
					int dy2 = (char)buf[5] * weight;

					fingers[id].add_delta(dx1, dy1, cnt == 1);
					if (sid > 0)
						fingers[sid].add_delta(dx2, dy2, cnt == 1);

					if (dy1 > 0 && dy2 > 0) {
						system("xdotool mousedown 4");
						system("xdotool mouseup 4");
					}
					if (dy1 < 0 && dy2 < 0) {
						system("xdotool mousedown 5");
						system("xdotool mouseup 5");
					}
					if (dx1 > 0 && dx2 > 0) {
						system("xdotool mousedown 6");
						system("xdotool mouseup 6"); }
					if (dx1 < 0 && dx2 < 0) {
						system("xdotool mousedown 7");
						system("xdotool mouseup 7");
					}


//					printf("%d: %d/%d %d: %d/%d\n", id, dx1, dy1, sid, dx2, dy2);
					break;
				}
			case STATUS:
				{
					int info = buf[1] & 0x1f;
					int lcnt = cnt;
					cnt = 0;
					for (int i=0; i<ETP_MAX_FINGERS; i++) {
						if ((info & (1<<i)) == 0) {
							fingers[i].release(clicked);
						} else {
							cnt++;
						}
					}
					if (lcnt == 0 && cnt > 0) {
						clicked = false;
					}
					printf("curr count: %d\n", cnt);
				}
		}
	}

	void parse_pkts() {
		while (true) {
			unsigned char buf[6];
			int len = read_data(buf, hw_ver > 1 ? 6 : 4);
			parse_pkt(buf, len);
		}
	}
};

int main() {
	Mouse m;
	m.open_dev();
	if (!m.detect())
		errexit("not Elan Touchpad");

	m.init();
	m.parse_pkts();
	m.dump_loop();
}
