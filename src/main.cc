#include <iostream>
#include "x.h"

int main(void) {
	XLib X{};

	/*int ret;
	char** fonts = XListFonts(X.display, "*", INT32_MAX, &ret);

	for(int i = 0; i < ret; i++) {
		std::cout << fonts[i] << std::endl;
	}

	exit(0);*/

	X.DrawTextAt(5, 5, U"Helloé, Worldλɢɹʈ");

	X.Run([&](XEvent& e) {
		switch (e.type) {
			case ConfigureNotify: {
				X.Redraw();
				break;
			}
		}
	});
}
