#include "config.h"
#include "mouse.h"

int main() {
	Mouse m;
	m.open_dev();
	if (!m.detect())
		errexit("not Elan Touchpad");

	m.init();
	m.parse_pkts();
	m.dump_loop();
}
