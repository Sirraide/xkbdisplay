#include "x.h"

#include <unistd.h>

#define FPS 120

XLib::XLib() {
	display = Verify(XOpenDisplay(nullptr), "XOpenDisplay()");
	screen	= XDefaultScreen(display);
	window	= XCreateSimpleWindow(display, XRootWindow(display, screen), 0, 0, w_width, w_height, 0, 0, 0x2D2A2E);
	white	= XWhitePixel(display, screen);
	black	= XBlackPixel(display, screen);
	// XSelectInput(display, window, KeyPressMask);
}

XLib::~XLib() {
	XCloseDisplay(display);
}

void XLib::Run(std::function<void(XEvent& e)> event_callback) {
	// Enable receiving of WM_DELETE_WINDOW
	delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(display, window, &delete_window, 1);

	// Display the window
	XMapWindow(display, window);
	XSync(display, False);

	// Allocate the GC
	XGCValues values{};
	values.cap_style  = CapRound;
	values.join_style = JoinRound;
	ulong value_mask  = GCCapStyle | GCJoinStyle;

	gc = Verify(XCreateGC(display, window, value_mask, &values), "XCreateGC()");

	// Set line params
	XSetForeground(display, gc, white);
	XSetBackground(display, gc, bgcolour);
	XSetFillStyle(display, gc, FillSolid);
	XSetLineAttributes(display, gc, 2, LineSolid, CapRound, JoinRound);

	// Generate stuff to draw
	GenerateGrid();

	bool quit = false;
	while (!quit) {
		while (XPending(display)) {
			XEvent e{};
			XNextEvent(display, &e);
			switch (e.type) {
				case ClientMessage: {
					if (static_cast<Atom>(e.xclient.data.l[0]) == delete_window) {
						quit = true;
						continue;
					}
				}
			}
			event_callback(e);
		}

		// Draw something
		Draw();

		usleep(1000000 / FPS);
	}
}

void XLib::Draw() {
	XDrawRectangles(display, window, gc, rects.data(), int(rects.size()));
}

void XLib::GenerateGrid() {
	for (short x = 40; x < short(w_width - 40); x += 50)
		for (short y = 40; y < short(w_height - 40); y += 50)
			rects.push_back(XRectangle{.x = x, .y = y, .width = 40, .height = 40});
}
