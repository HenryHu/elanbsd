#ifndef _ELANBSD_MOUSE_H_
#define _ELANBSD_MOUSE_H_


#include "xdisplay.h"
#include "elanbsd.h"
#include "config.h"
#include "finger.h"
#include "button.h"

#include <string>
#include <fcntl.h>
#include <sys/mouse.h>
#include <math.h>

class Mouse {
	int ver_major, ver_minor, ver_step;
	int x_max, y_max, width;
	int max_cnt;
	int hw_ver;
	int cnt;
	int fd;
	int ctrl_code;
	bool clicked;
	int touch_num, last_touch_num;
	int scroll_x_test, scroll_y_test;
	bool in_three_drag;
	double two_finger_dist, three_drag_test;
	int max_pres;
	int main_finger_id;
	unsigned char cap[3];
	Finger fingers[ETP_MAX_FINGERS];
	Button btns[ETP_MAX_BUTTONS];

	void check(int ret, const std::string& err) {
		if (ret < 0) errexit(err);
	}
	XDisplay dpy;

public:
	// physical distance (mm)
	inline int get_phy_dist(int dist) {
		return dist * width / x_max;
	}
	Mouse() {
		for (int i=0; i<ETP_MAX_FINGERS; i++) {
			fingers[i].set_id(i);
			fingers[i].set_dpy(&dpy);
		}
		for (int i=0; i<sizeof(btns) / sizeof(Button); i++) {
			btns[i].set_id(i);
			btns[i].set_dpy(&dpy);
		}
		two_finger_dist = -2;
		ctrl_code = dpy.get_keycode("Control_L");
		touch_num = 0;
		last_touch_num = 0;
		in_three_drag = false;
		max_pres = 0;
	}
	void open_dev() {
		mousemode_t info;
		// open blocking mode device
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
		// it's unclear why read cannot read them all at once
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
	// both param & result are stored in buf
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
		if (buf[0] == 0x3c && buf[1] == 0x03 && (buf[2] == 0x00 || buf[2] == 0xc8)) {
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
			case 4:
				hw_ver = 2;
				break;
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
			case 2:
				send_cmd(ETP_PS2_CUSTOM_COMMAND);
				send_cmd(ETP_REGISTER_WRITE);
				send_cmd(ETP_PS2_CUSTOM_COMMAND);
				send_cmd((unsigned char)reg);
				send_cmd(ETP_PS2_CUSTOM_COMMAND);
				send_cmd(val);
				send_cmd(PSMC_SET_SCALING11);
				break;
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
			case 2:
				write_reg(0x10, 0x54);
				write_reg(0x11, 0x88);
				write_reg(0x21, 0x60);
				break;
			case 4:
				write_reg(7, 1);
				break;
		}
	}
	void get_range() {
		unsigned char buf[3];
		switch (hw_ver) {
			case 2:
				if (ver_major == 2 && (
						((ver_minor == 0x08) && (ver_step == 0x00)) ||
						((ver_minor == 0x0b) && (ver_step == 0x00)) ||
						((ver_minor == 0x00) && (ver_step == 0x30)))) {
					x_max = ETP_XMAX_V2;
					y_max = ETP_YMAX_V2;
				} else {
					if (ver_major == 4 && ver_minor == 0x02 && ver_step ==0x16) {
						x_max = 819;
						y_max = 405;
					} else if (ver_major == 4 && ver_minor == 0x02 && (ver_step == 0x19 || ver_step == 0x15)) {
						x_max = 900;
						y_max = 500;
					} else {
						int max_off;

						if (ver_major == 2 && ver_minor == 0x08 && ver_step > 0) {
							max_off = 1;
						} else {
							max_off = 2;
						}

						if (ver_major == 0x14) {
							send_adv_cmd(ETP_FW_ID_QUERY, buf);
							bool fixed_dpi = buf[1] & 0x10;
							if (fixed_dpi) {
								send_adv_cmd(ETP_SAMPLE_QUERY, buf);
								x_max = (cap[1] - max_off) * buf[1] / 2;
								y_max = (cap[2] - max_off) * buf[2] / 2;
								break;
							}
						}

						x_max = (cap[1] - max_off) * 64;
						y_max = (cap[2] - max_off) * 64;
					}
				}
				width = ETP_WIDTH_V2;
				break;
		break;
			case 4:
				send_adv_cmd(ETP_FW_ID_QUERY, buf);
				x_max = (0x0f & buf[0]) << 8 | buf[1];
				y_max = (0xf0 & buf[0]) << 4 | buf[2];
				width = x_max / (cap[1] - 1);
				break;
		}
		for (int i=0; i<ETP_MAX_FINGERS; i++)
			fingers[i].set_range(x_max, y_max, width);
		scroll_x_test = x_max / 40;
		scroll_y_test = y_max / 40;
		three_drag_test = sqrt(x_max * x_max + y_max * y_max) / 20;
		printf("x_max: %d y_max: %d width: %d\n", x_max, y_max, width);
	}
	void enable() {
		send_cmd(PSMC_ENABLE_DEV);
	}
	void update_touch_num() {
		touch_num = 0;
		for (int i=0; i<ETP_MAX_FINGERS; i++)
			if (fingers[i].is_touched())
				touch_num++;
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

	bool pres_too_low(int pres) {
		int limit = PRESSURE_LIMIT;
		if (pres <= limit) {
			printf("pressure %d <= limit %d (max %d)\n", pres, limit, max_pres);
			return true;
		}
		if (pres > max_pres)
			max_pres = pres;
		return false;
	}

	void parse_v2(unsigned char *buf, int len) {
		int tcnt = cnt;
		cnt = (buf[0] & 0xc0) >> 6;

		btns[0].update(buf[0] & 0x01);
		btns[2].update(buf[0] & 0x02);
		if (btns[0].is_down() || btns[2].is_down()) {
			clicked = true;
		}

		switch (cnt) {
			case 3:
				if (buf[3] & 0x80) {
					cnt = 4;
					fingers[3].touch();
				} else {
					fingers[3].release(clicked, tcnt, max_cnt);
				}
			case 1:
				{
					int x = ((buf[1] & 0x0f) << 8 | buf[2]);
					int y = ((buf[4] & 0x0f) << 8 | buf[5]);
					int pres = (buf[1] & 0xf0) | ((buf[4] & 0xf0) >> 4);
					int width = ((buf[0] & 0x30) >> 2) | ((buf[3] & 0x30) >> 4);
					printf("[%d] x: %d y: %d pres: %d width: %d main: %d\n", cnt, x, y, pres, width, main_finger_id);
					if (pres_too_low(pres)) {
						break;
					}
					int old_id = main_finger_id;
					if (fingers[0].is_touched() && fingers[1].is_touched()) {
						Finger tmp_finger;
						tmp_finger.set_pos(x, y, false);
						if (fingers[0].dist(tmp_finger) < fingers[1].dist(tmp_finger)) {
							main_finger_id = 0;
						} else {
							main_finger_id = 1;
						}
					} else if (!fingers[0].is_touched() && !fingers[1].is_touched()) {
						main_finger_id = 0;
					}
					if (old_id != main_finger_id)
						printf("\tupdated main: %d\n", main_finger_id);
					fingers[main_finger_id].set_info(pres, width);
					fingers[main_finger_id].set_pos(x, y, true);
					fingers[main_finger_id].touch();
					if (cnt >= 3) {
						fingers[1-main_finger_id].touch();
						fingers[2].touch();
					} else {
						fingers[1-main_finger_id].release(clicked, tcnt, max_cnt);
						fingers[2].release(clicked, tcnt, max_cnt);
					}

					break;
				}
			case 2:
				{
					int x1 = ((buf[0] & 0x10) << 4 | buf[1]) << 2;
					int y1 = ((buf[0] & 0x20) << 3 | buf[2]) << 2;
					int x2 = ((buf[3] & 0x10) << 4 | buf[4]) << 2;
					int y2 = ((buf[3] & 0x20) << 3 | buf[5]) << 2;
					printf("[2] x1: %d y1: %d x2: %d y2:%d main: %d\n", x1, y1, x2, y2, main_finger_id);
					if (main_finger_id < 0) main_finger_id = 0;
					fingers[main_finger_id].set_pos(x1, y1, false);
					fingers[main_finger_id].touch();
					fingers[1-main_finger_id].set_pos(x2, y2, false);
					fingers[1-main_finger_id].touch();
					fingers[2].release(clicked, tcnt, max_cnt);
					fingers[3].release(clicked, tcnt, max_cnt);
					break;
				}
			case 0:
				{
					printf("[0] all gone\n");
					main_finger_id = -1;
					fingers[0].release(clicked, tcnt, max_cnt);
					fingers[1].release(clicked, tcnt, max_cnt);
					fingers[2].release(clicked, tcnt, max_cnt);
					fingers[3].release(clicked, tcnt, max_cnt);
					break;
				}
		}
		update_touch_num();
	}

	void parse_v4(unsigned char *buf, int len) {
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
					if (pres_too_low(pres)) {
						break;
					}
					fingers[id].set_info(pres, traces);
					fingers[id].set_pos(x, y, (cnt == 1) | in_three_drag);
					fingers[id].touch();
					update_touch_num();

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

					fingers[id].add_delta(dx1, dy1, (cnt == 1) || in_three_drag);
					if (sid > 0)
						fingers[sid].add_delta(dx2, dy2, (cnt == 1) || in_three_drag);

//					printf("%d: %d/%d %d: %d/%d\n", id, dx1, dy1, sid, dx2, dy2);
					break;
				}
			case STATUS:
				{
					int info = buf[1] & 0x1f;
					int tcnt = cnt;
					cnt = 0;
					for (int i=0; i<ETP_MAX_FINGERS; i++) {
						if ((info & (1<<i)) == 0) {
							fingers[i].release(clicked, tcnt, max_cnt);
						} else {
							cnt++;
						}
					}
					update_touch_num();
					printf("curr count: %d / %d\n", cnt, tcnt);
				}
		}

	}

	void parse_pkt(unsigned char *buf, int len) {
		int lcnt = cnt;
		switch (hw_ver) {
			case 2:
				parse_v2(buf, len);
				break;
			case 4:
				parse_v4(buf, len);
				break;
		}
		if (lcnt == 0 && cnt > 0) {
			clicked = false;
		}

		if (cnt > max_cnt) {
			max_cnt = cnt;
		} else if (cnt == 0) {
			max_cnt = 0;
		}

		if (touch_num == 2) {
			int f1 = -1;
			int f2 = -1;
			for (int i=0; i<ETP_MAX_FINGERS; i++) {
				if (fingers[i].is_touched()) {
					if (f1 == -1) f1 = i;
					else {
						f2 = i;
						break;
					}
				}
			}

			int dx1, dx2, dy1, dy2;
			if (!fingers[f1].get_delta(dx1, dy1) || !fingers[f2].get_delta(dx2, dy2)) {
				fingers[f1].save_pos();
				fingers[f2].save_pos();
			} else {
				if (dy1 > scroll_y_test && dy2 > scroll_y_test) {
					dpy.click(3);
					fingers[f1].save_pos();
					fingers[f2].save_pos();
				}
				if (dy1 < -scroll_y_test && dy2 < -scroll_y_test) {
					dpy.click(4);
					fingers[f1].save_pos();
					fingers[f2].save_pos();
				}
				if (dx1 > scroll_x_test && dx2 > scroll_x_test) {
					dpy.click(5);
					fingers[f1].save_pos();
					fingers[f2].save_pos();
				}
				if (dx1 < -scroll_x_test && dx2 < -scroll_x_test) {
					dpy.click(6);
					fingers[f1].save_pos();
					fingers[f2].save_pos();
				}
			}

			double cur_dist = fingers[f1].dist(fingers[f2]);
			if (two_finger_dist > -1) {
				if (cur_dist > two_finger_dist + x_max / 20) {
					dpy.key_down(37);
					dpy.click(3);
					dpy.key_up(37);
					two_finger_dist = cur_dist;
				} else if (cur_dist < two_finger_dist - x_max / 20) {
					dpy.key_down(37);
					dpy.click(4);
					dpy.key_up(37);
					two_finger_dist = cur_dist;
				}
			} else {
				two_finger_dist = cur_dist;
			}
			printf("last dist: %lf\n", two_finger_dist);
			printf("dist: %lf\n", cur_dist);
		} else {
			two_finger_dist = -2;
		}
		if (touch_num == 3) {
			if (last_touch_num < 3) {

			}

			int f1, f2, f3;
			f1 = f2 = f3 = -1;
			for (int i=0; i<ETP_MAX_FINGERS; i++) {
				if (fingers[i].is_touched()) {
					if (f1 < 0) f1 = i;
					else if (f2 < 0) f2 = i;
					else {
						f3 = i;
						break;
					}
				}
			}
			double d1, d2, d3;
			if (!fingers[f1].get_delta_dist(d1) || !fingers[f2].get_delta_dist(d2) || !fingers[f3].get_delta_dist(d3)) {
				fingers[f1].save_pos();
				fingers[f2].save_pos();
				fingers[f3].save_pos();
			} else {
				if (d1 > three_drag_test || d2 > three_drag_test || d3 > three_drag_test) {
					if (!in_three_drag) {
						dpy.press(true, 0);
						in_three_drag = true;
					}
				}
			}
		}
		if (touch_num == 0) {
			if (in_three_drag) {
				in_three_drag = false;
				dpy.press(false, 0);
			}
		}
		last_touch_num = touch_num;
	}

	void parse_pkts() {
		while (true) {
			unsigned char buf[6];
			int len = read_data(buf, hw_ver > 1 ? 6 : 4);
			parse_pkt(buf, len);
		}
	}
};


#endif // _ELANBSD_MOUSE_H_
