#include <iostream>
#include "x.h"
#include <clocale>

int main(void) {
	setlocale(LC_ALL, "");
	XLib X{};

	X.Run([&](XEvent& e) {
		switch (e.type) {
			case ConfigureNotify: {
				X.Redraw();
				break;
			}
		}
	});
}
