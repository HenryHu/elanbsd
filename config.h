#ifndef _ELANBSD_CONFIG_H_
#define _ELANBSD_CONFIG_H_

#define USE_XTEST

const int PRESSURE_LIMIT = 15; // range: 0~255 (v4)

const int ZOOM_DIST_LIMIT = 6; // zoom step physical limit (mm)
const int SCROLL_X_DIST_LIMIT = 4; // physical x dist to scroll (mm)
const int SCROLL_Y_DIST_LIMIT = 4; // physical y dist to scroll (mm)
const int VSCROLL_LIMIT = 1; // physical y dist to trigger a vscroll (mm)
const int THREE_DRAG_DIST_LIMIT = 4; // physical dist step for three-finger drag (mm)

const double MOVE_X_SCALE = 1.0; // x scale during movement, range: (0, inf)
const double MOVE_Y_SCALE = 1.0; // y scale during movement, range: (0, inf)

/*
 *  +-----+ <
 *  |     | .
 *  |  X  | < TAP_Y_LIMIT
 *  |A    |
 *  +-----+
 *     ^..^ TAP_X_LIMIT
 *   B
 *
 *  X: touch point
 *  release in B: not a tap
 *  release in A: is a tap
 *
 */
const int TAP_X_LIMIT = 2; // max movement allowed in x for a tap (mm)
const int TAP_Y_LIMIT = 2; // max movement allowed in y for a tap (mm)

/*
 * +------------------------+----+
 * |                        |    |
 * |                        |    |  A: vscroll area
 * |           B            | B  |  B: tap would click button 0
 * |                        |    |  C: tap would click button 1
 * |                        | A  |  D: tap would click button 2
 * |                        |    |
 * +-----------+------+-----+----+  < BTN_RANGE_Y
 * |           |      |     | A  |
 * |     B     |   C  |   D | D  |
 * +-----------+------+-----+----+
 *                          ^ VSCROLL_RANGE
 *             ^ BTN_0_RANGE_X
 *                    ^ BTN_2_RANGE_X
 */
const double VSCROLL_RANGE = 0.9; // x range for vscroll range: (0, 1)
const double BTN_RANGE_Y = 0.2; // y range for button click range: (0, 1)
const double BTN_0_RANGE_X = 0.4; // x range for button 0 range: (0, 1)
const double BTN_2_RANGE_X = 0.6; // x range for button 2 range: (0, 1)

const int TAP_TIMEOUT = 100; // max touching time for a tap (ms)
const int TAP_TO_DRAG_TIMEOUT = 200; // tap-to-drag timeout (ms)

const double MOVE_DX_LIMIT = 0.1; // minimum physical x movement to trigger movement (mm)
const double MOVE_DY_LIMIT = 0.1; // minimum physical y movement to trigger movement (mm)

const int HISTORY_LEN = 20; // number of data points to store
const int AVG_WINDOW_SIZE = 4; // number of data points used to calculate average
const int AVG_WINDOW[] = {10, 8, 3, -1};

const int ACCEL_SCALE = 100;
const int ACCEL_ACCELERATION = 10;

#endif //  _ELANBSD_CONFIG_H_
