#ifndef _ELANBSD_CONFIG_H_
#define _ELANBSD_CONFIG_H_

#define USE_XTEST

const int PRESSURE_LIMIT = 15; // range: 0~255 (v4)
const int TAP_TO_DRAG_TIMEOUT = 200; // tap-to-drag timeout (ms)
const int ZOOM_DIST_LIMIT = 6; // zoom step physical limit (mm)
const int SCROLL_X_DIST_LIMIT = 4; // physical x dist to scroll (mm)
const int SCROLL_Y_DIST_LIMIT = 4; // physical y dist to scroll (mm)
const int THREE_DRAG_DIST_LIMIT = 4; // physical dist step for three-finger drag (mm)

#endif //  _ELANBSD_CONFIG_H_
