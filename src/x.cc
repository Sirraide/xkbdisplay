#include "x.h"

#include <unistd.h>

#define FPS			  120
#define KEYS_IN_ROW_1 13
#define KEYS_IN_ROW_2 12
#define KEYS_IN_ROW_3 12
#define KEYS_IN_ROW_4 11

XLib::XLib() {
	display = Verify(XOpenDisplay(nullptr), "XOpenDisplay()");

	// Set up error handler
	XSetErrorHandler(HandleError);

	screen = XDefaultScreen(display);
	window = XCreateSimpleWindow(display, XRootWindow(display, screen), 0, 0, w_width, w_height, 0, 0, 0x2D2A2E);
	white  = XWhitePixel(display, screen);
	black  = XBlackPixel(display, screen);
	// XSelectInput(display, window, KeyPressMask);

	XGetWindowAttributes(display, window, &attrs);

	// Initialise the font
	/*font = Verify(XftFontOpen(display, screen,
					  XFT_FAMILY, XftTypeString, "Charis SIL",
					  XFT_SIZE, XftTypeDouble, double(w(.01)),
					  nullptr),
		"XftFontOpen()");
*/
	draw	 = Verify(XftDrawCreate(display, window, attrs.visual, attrs.colormap), "XftDrawCreate()");
	x_colour = XColour(0xFCFCFA);
	XftColorAllocValue(display, attrs.visual, attrs.colormap, &x_colour, &xft_colour);
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

	// Set a min size
	auto* hints		  = XAllocSizeHints();
	hints->min_height = static_cast<int>(base_height / 2);
	hints->min_width  = static_cast<int>(base_width / 2);
	hints->flags	  = PMinSize | PAspect;
	hints->min_aspect = hints->max_aspect = {.x = base_width, .y = base_height};
	XSetWMNormalHints(display, window, hints);

	// Set the window name
	XStoreName(display, window, "XKBDisplay");

	// Allocate the GC
	XGCValues values{};
	values.cap_style  = CapRound;
	values.join_style = JoinRound;
	ulong value_mask  = GCCapStyle | GCJoinStyle;

	gc = Verify(XCreateGC(display, window, value_mask, &values), "XCreateGC()");

	// Set line params
	XSetForeground(display, gc, fgcolour);
	XSetBackground(display, gc, bgcolour);
	XSetFillStyle(display, gc, FillSolid);
	XSetLineAttributes(display, gc, base_line_width, LineSolid, CapRound, JoinRound);

	// Subscribe to events
	XSelectInput(display, window, StructureNotifyMask);

	// Initialise the window content
	Redraw();

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
		/*XDrawString(display, window, gc, static_cast<int>(xpos), 10, hello.data(), int(hello.size()));*/
		usleep(1000000 / FPS);
	}
}

void XLib::Draw() {
	XDrawRectangles(display, window, gc, rects.data(), int(rects.size()));
}

void XLib::GenerateGrid() {
	const ushort step		= w(.06);
	const ushort sstep		= w(.01);
	const ushort top_offset = w(.02);
	for (ushort x = sstep, i = 0; x < ushort(w_width - step) && i < KEYS_IN_ROW_1; x += step + sstep, i++)
		rects.push_back(XRectangle{.x = short(x), .y = short(sstep + top_offset), .width = step, .height = step});
	for (ushort x = 2 * step, i = 0; x < ushort(w_width - step) && i < KEYS_IN_ROW_2; x += step + sstep, i++)
		rects.push_back(XRectangle{.x = short(x), .y = short(2 * sstep + step + top_offset), .width = step, .height = step});
	for (ushort x = 2 * step + 2 * sstep, i = 0; x < ushort(w_width - step) && i < KEYS_IN_ROW_3; x += step + sstep, i++)
		rects.push_back(XRectangle{.x = short(x), .y = short(3 * sstep + 2 * step + top_offset), .width = step, .height = step});
	for (ushort x = 2 * step + 4 * sstep, i = 0; x < ushort(w_width - step) && i < KEYS_IN_ROW_4; x += step + sstep, i++)
		rects.push_back(XRectangle{.x = short(x), .y = short(4 * sstep + 3 * step + top_offset), .width = step, .height = step});
}

void XLib::Redraw() {
	XGetWindowAttributes(display, window, &attrs);
	w_width	 = static_cast<uint>(attrs.width);
	w_height = static_cast<uint>(attrs.height);
	XSetLineAttributes(display, gc, uint(base_line_width * double(w_width) / double(base_width)), LineSolid, CapRound, JoinRound);
	rects.clear();
	GenerateGrid();
	XClearWindow(display, window);

	font_sz = w(.015);
	if (main_font_cache.contains(font_sz)) font = main_font_cache[font_sz];
	else {
		font = main_font_cache[font_sz] = Verify(XftFontOpen(display, screen,
													 XFT_FAMILY, XftTypeString, "Charis SIL",
													 XFT_SIZE, XftTypeDouble, font_sz,
													 nullptr),
			"XftFontOpen(): Could not open Charis SIL " + std::to_string(font_sz));
	}

	Draw();
	DrawTextElems();
}

int XLib::HandleError(Display* display, XErrorEvent* e) {
	static char buffer[2048];
	XGetErrorText(display, e->error_code, buffer, 2048);
	Die(buffer);
}

void XLib::DrawTextAt(int x, int y, std::u32string text) {
	text_elems.push_back({x, y, std::move(text)});
}

void XLib::DrawTextElems() {
	for (auto& text_elem : text_elems)
		XftDrawString32(draw, &xft_colour, font, text_elem.x * w(.01), text_elem.y * w(.01),
			(const FcChar32*) text_elem.content.data(), int(text_elem.content.size()));
}
