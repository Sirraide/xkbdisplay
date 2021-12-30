#include <iostream>
#include "x.h"
#include "layout.h"

int main(void) {
	XLib X{};

	Layout layout(&X);

	X.Run([&](XEvent& e) {
		switch (e.type) {
			case ConfigureNotify: {
				X.Redraw();
				break;
			}
		}
	});
}
