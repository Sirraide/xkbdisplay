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
	x_fgcolour = XColour(fgcolour);
	x_grey = XColour(grey);
	XftColorAllocValue(display, attrs.visual, attrs.colormap, &x_fgcolour, &xft_fgcolour);
	XftColorAllocValue(display, attrs.visual, attrs.colormap, &x_grey, &xft_grey);
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
		// Draw();
		/*XDrawString(display, window, gc, static_cast<int>(xpos), 10, hello.data(), int(hello.size()));*/
		usleep(1000000 / FPS);
	}
}

void XLib::Draw() {
	XDrawRectangles(display, window, gc, rects.data(), int(rects.size()));
	DrawTextElems(menu_text, &xft_fgcolour);
}

void XLib::GenerateKeyboard() {
	static const char32_t label_chars[] = U"¬1234567890-=QWERTYUIOP[]ASDFGHJKL;'^´ZXCVBNM,./";
	const ushort step		= w(.0625);
	const ushort sstep		= w(.0125);
	const ushort top_offset = w(.05);
	for (ushort x = sstep, i = 0; x < ushort(w_width - step) && i < KEYS_IN_ROW_1; x += step + sstep, i++)
		rects.push_back({.x = short(x), .y = short(sstep + top_offset), .width = step, .height = step});
	for (ushort x = step, i = 0; x < ushort(w_width - step) && i < KEYS_IN_ROW_2; x += step + sstep, i++)
		rects.push_back({.x = short(x), .y = short(2 * sstep + step + top_offset), .width = step, .height = step});
	for (ushort x = step + 2 * sstep, i = 0; x < ushort(w_width - step) && i < KEYS_IN_ROW_3; x += step + sstep, i++)
		rects.push_back({.x = short(x), .y = short(3 * sstep + 2 * step + top_offset), .width = step, .height = step});
	for (ushort x = step + 4 * sstep, i = 0; x < ushort(w_width - step) && i < KEYS_IN_ROW_4; x += step + sstep, i++)
		rects.push_back({.x = short(x), .y = short(4 * sstep + 3 * step + top_offset), .width = step, .height = step});

	for (size_t i = 0; i < rects.size(); i++) {
		auto& rect = rects[i];
		std::u32string text{label_chars[i]};
		auto extents = TextExtents(text);
		auto xpos = rect.x + rect.width / 2 - extents.width / 2;
		auto ypos = rect.y + rect.height / 2 + extents.height / 2;
		if(text == U"Q") ypos -= font->descent / 2;
		labels.push_back(Text{.x = xpos, .y = ypos, .content = text});
	}
}

void XLib::Redraw() {
	XGetWindowAttributes(display, window, &attrs);
	w_width	 = static_cast<uint>(attrs.width);
	w_height = static_cast<uint>(attrs.height);
	XSetLineAttributes(display, gc, uint(base_line_width * double(w_width) / double(base_width)), LineSolid, CapRound, JoinRound);
	rects.clear();
	labels.clear();

	font_sz = w(.015);
	if (main_font_cache.contains(font_sz)) font = main_font_cache[font_sz];
	else {
		font = main_font_cache[font_sz] = Verify(XftFontOpen(display, screen,
													 XFT_FAMILY, XftTypeString, "Charis SIL",
													 XFT_SIZE, XftTypeDouble, font_sz,
													 nullptr),
			"XftFontOpen(): Could not open Charis SIL " + std::to_string(font_sz));
	}

	GenerateKeyboard();
	GenerateMenuText();
	XClearWindow(display, window);
	Draw();
	DrawTextElems(labels, &xft_grey);
}

int XLib::HandleError(Display* display, XErrorEvent* e) {
	static char buffer[2048];
	XGetErrorText(display, e->error_code, buffer, 2048);
	Die(buffer);
}

void XLib::DrawTextAt(int x, int y, std::u32string text) {
	menu_text.push_back({x, y, std::move(text)});
}

void XLib::DrawTextElems(const std::vector<Text>& text_elems, XftColor* colour) const {
	for (auto& text_elem : text_elems)
		XftDrawString32(draw, colour, font, text_elem.x, text_elem.y,
			(const FcChar32*) text_elem.content.data(), int(text_elem.content.size()));
}
XGlyphInfo XLib::TextExtents(const std::u32string& t) const {
	XGlyphInfo extents;
	XftTextExtents32(display, font, (const FcChar32*) t.data(), int(t.size()), &extents);
	return extents;
}

void XLib::GenerateMenuText() {
	menu_text.clear();
	std::u32string font_name = U"Font: Charis SIL " + ToUTF32(std::to_string(int(font_sz)));
	auto extents = TextExtents(font_name);
	auto xpos = w_width / 2 - extents.width / 2;
	auto ypos = extents.height * 2;
	DrawTextAt(xpos, ypos, font_name);
}
