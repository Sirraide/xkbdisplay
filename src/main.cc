#include <iostream>
#include "x.h"
#include "layout.h"
#include <X11/XKBlib.h>
#include <clocale>

int main(void) {
	setlocale(LC_ALL, "");
	XLib X{};

	char buf[2048];
	KeySym sym;

	XKeyPressedEvent ev;
	ev.type = KeyPress;
	ev.display = X.display;
	ev.keycode = X.cells[0].keycode_raw;
	ev.state = 0;
	XLookupString(&ev, buf, 2048, &sym, nullptr);
	std::cout << buf << std::endl;
	exit(0);

	X.Run([&](XEvent& e) {
		switch (e.type) {
			case ConfigureNotify: {
				X.Redraw();
				break;
			}
		}
	});
}
