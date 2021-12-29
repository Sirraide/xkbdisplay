#ifndef XKBDISPLAY_X_H
#define XKBDISPLAY_X_H

#include "utils.h"

#include <X11/Xlib.h>
#include <functional>
#include <vector>

struct XLib {
	Display* display{};
	Window	 window{};
	Atom	 delete_window{};
	GC		 gc{};
	int screen{};

	static constexpr ulong bgcolour = 0x2D2A2E;

	unsigned w_width  = 800;
	unsigned w_height = 600;

	ulong white{};
	ulong black{};

	void Draw();
	void GenerateGrid();
	void Run(std::function<void(XEvent& e)> event_callback);

	std::vector<XRectangle> rects{};

	XLib();
	~XLib();
};

#endif // XKBDISPLAY_X_H
