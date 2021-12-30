#include "x.h"

#include <unistd.h>

#define FPS 120

XLib::XLib() {
	display	  = Verify(XOpenDisplay(nullptr), "XOpenDisplay()");
	connexion = Verify(XGetXCBConnection(display), "XGetXCBConnection()");
	// Set up error handler
	XSetErrorHandler(HandleError);

	screen = XDefaultScreen(display);
	window = XCreateSimpleWindow(display, XRootWindow(display, screen), 0, 0, w_width, w_height, 0, 0, 0x2D2A2E);
	white  = XWhitePixel(display, screen);
	black  = XBlackPixel(display, screen);

	XGetWindowAttributes(display, window, &attrs);

	// Set up colours
	draw	   = Verify(XftDrawCreate(display, window, attrs.visual, attrs.colormap), "XftDrawCreate()");
	x_fgcolour = XColour(fgcolour);
	x_grey	   = XColour(grey);
	XftColorAllocValue(display, attrs.visual, attrs.colormap, &x_fgcolour, &xft_fgcolour);
	XftColorAllocValue(display, attrs.visual, attrs.colormap, &x_grey, &xft_grey);

	InitCells();
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

		// DrawCells something
		// DrawCells();
		/*XDrawString(display, window, gc, static_cast<int>(xpos), 10, hello.data(), int(hello.size()));*/
		usleep(1000000 / FPS);
	}
}

void XLib::DrawCells() {
	XDrawRectangles(display, window, gc, cell_borders.data(), int(cell_borders.size()));
	for (const auto& cell : cells) {
		DrawTextElem(cell.label, &xft_grey);
		DrawTextElem(cell.keycode, &xft_grey, Font("Charis SIL", font_sz / 2));
		DrawTextElems(cell.keysyms, &xft_fgcolour, Font("Charis SIL", ushort(font_sz / 1.33)));
	}
	DrawTextElems(menu_text, &xft_fgcolour);
}

void XLib::GenerateKeyboard() {
	const ushort step		= w(.0625);
	const ushort sstep		= w(.0125);
	const ushort top_offset = w(.05);

	// Figure out where to place the cells and set up the borders around them
	size_t cell_index = 0;
	for (ushort x = sstep, i = 0; x < ushort(w_width - step) && i < KEYS_IN_ROW_1; x += step + sstep, i++)
		*cells[cell_index++].border = {.x = short(x), .y = short(sstep + top_offset), .width = step, .height = step};
	for (ushort x = step, i = 0; x < ushort(w_width - step) && i < KEYS_IN_ROW_2; x += step + sstep, i++)
		*cells[cell_index++].border = {.x = short(x), .y = short(2 * sstep + step + top_offset), .width = step, .height = step};
	for (ushort x = step + 2 * sstep, i = 0; x < ushort(w_width - step) && i < KEYS_IN_ROW_3; x += step + sstep, i++)
		*cells[cell_index++].border = {.x = short(x), .y = short(3 * sstep + 2 * step + top_offset), .width = step, .height = step};
	for (ushort x = step + 4 * sstep, i = 0; x < ushort(w_width - step) && i < KEYS_IN_ROW_4; x += step + sstep, i++)
		*cells[cell_index++].border = {.x = short(x), .y = short(4 * sstep + 3 * step + top_offset), .width = step, .height = step};

	// Set up the cell labels, keycodes, and keysyms
	cell_index = 0;
	for (size_t i = 0; i < KEY_COUNT; i++) {
		auto&		   cell = cells[cell_index++];
		auto*		   rect = cell.border;
		std::u32string text{cell.label_char};
		auto		   extents = TextExtents(text);
		auto		   xpos	   = rect->x + rect->width / 2 - extents.width / 2;
		auto		   ypos	   = rect->y + rect->height / 2 + extents.height / 2;
		if (cell.label_char == U'Q') ypos -= font->descent / 2;
		cell.label = {.x = xpos, .y = ypos, .content = text};

		cell.keycode.x = xpos;
		cell.keycode.y = cell.border->y + font_sz / 2 + 4;

		const auto vstride = ushort(.23 * rect->width);
		const auto hstride = ushort(.25 * rect->height);

		const auto v_start = rect->x + ushort(.1 * rect->width);

		cell.keysyms[0].x = cell.keysyms[1].x = v_start;
		cell.keysyms[2].x = cell.keysyms[3].x = v_start + vstride;
		cell.keysyms[4].x = cell.keysyms[5].x = v_start + vstride * 2;
		cell.keysyms[6].x = cell.keysyms[7].x = v_start + vstride * 3;

		cell.keysyms[0].y	  = cell.keysyms[2].y =
			cell.keysyms[4].y = cell.keysyms[6].y = rect->y + rect->height - hstride;

		cell.keysyms[1].y	  = cell.keysyms[3].y =
			cell.keysyms[5].y = cell.keysyms[7].y = rect->y + hstride;
	}
}

void XLib::Redraw() {
	XGetWindowAttributes(display, window, &attrs);
	w_width	 = static_cast<uint>(attrs.width);
	w_height = static_cast<uint>(attrs.height);
	XSetLineAttributes(display, gc, uint(base_line_width * double(w_width) / double(base_width)), LineSolid, CapRound, JoinRound);

	font_sz = w(.015);
	font	= Font("Charis SIL", font_sz);

	GenerateKeyboard();
	GenerateMenuText();
	XClearWindow(display, window);
	DrawCells();
}

int XLib::HandleError(Display* display, XErrorEvent* e) {
	static char buffer[2048];
	XGetErrorText(display, e->error_code, buffer, 2048);
	Die(buffer);
}

void XLib::DrawTextAt(int x, int y, std::u32string text) {
	menu_text.push_back({x, y, std::move(text)});
}

void XLib::DrawTextElems(const auto& text_elems, XftColor* colour, XftFont* fnt) const {
	for (auto& text_elem : text_elems) DrawTextElem(text_elem, colour, fnt);
}

void XLib::DrawTextElem(const Text& elem, XftColor* colour, XftFont* fnt) const {
	if (elem.content.empty()) return;
	if (fnt == nullptr) fnt = font;
	XftDrawString32(draw, colour, fnt, elem.x, elem.y,
		(const FcChar32*) elem.content.data(), int(elem.content.size()));
}

XGlyphInfo XLib::TextExtents(const std::u32string& t, XftFont* fnt) const {
	if (fnt == nullptr) fnt = font;
	XGlyphInfo extents;
	XftTextExtents32(display, fnt, (const FcChar32*) t.data(), int(t.size()), &extents);
	return extents;
}

void XLib::GenerateMenuText() {
	menu_text.clear();
	std::u32string font_name = U"Font: Charis SIL " + ToUTF32(std::to_string(int(font_sz)));
	DrawCentredTextAt(font_name, w_width / 2, HEIGHT_TIMES_TWO);
}

void XLib::DrawCentredTextAt(const std::u32string& text, int xpos, int ypos) {
	auto extents = TextExtents(text);
	auto x		 = xpos - extents.width / 2;
	auto y		 = ypos == HEIGHT_TIMES_TWO ? extents.height * 2 : ypos;
	DrawTextAt(int(x), int(y), text);
}

void XLib::InitCells() {
	static constexpr char32_t label_chars[KEY_COUNT + 1] = U"¬1234567890-=QWERTYUIOP[]ASDFGHJKL;'^´ZXCVBNM,./"; // +1 because \0
	static constexpr KeyCode  keycodes[KEY_COUNT]		 = {
		49, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
		24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
		38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 51,
		94, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61};
	auto* borders = cell_borders.data();
	for (size_t i = 0; i < KEY_COUNT; i++) {
		cells[i].label_char		 = label_chars[i];
		cells[i].border			 = borders + i;
		cells[i].keycode_raw	 = keycodes[i];
		cells[i].keycode.content = ToUTF32(std::to_string(keycodes[i]));
		for (size_t j = 0; j < 8; j++) GetKeysyms(cells[i]);
	}
}

XftFont* XLib::Font(const std::string& name, ushort font_sz) {
	if (font_cache.contains({name, font_sz})) return font_cache[{name, font_sz}];
	else {
		return font_cache[{name, font_sz}] = Verify(XftFontOpen(display, screen,
														XFT_FAMILY, XftTypeString, name.data(),
														XFT_SIZE, XftTypeDouble, double(font_sz),
														nullptr),
				   "XftFontOpen(): Could not open " + name + " " + std::to_string(font_sz));
	}
}
void XLib::GetKeysyms(Cell& cell) {
	cell.keysyms[0].content = ResolveKeysym(cell.keycode_raw, 0);
	cell.keysyms[1].content = ResolveKeysym(cell.keycode_raw, ShiftMask);
	cell.keysyms[2].content = ResolveKeysym(cell.keycode_raw, Mod5Mask);
	cell.keysyms[3].content = ResolveKeysym(cell.keycode_raw, ShiftMask | Mod5Mask);
	cell.keysyms[4].content = ResolveKeysym(cell.keycode_raw, Mod3Mask);
	cell.keysyms[5].content = ResolveKeysym(cell.keycode_raw, ShiftMask | Mod3Mask);
	cell.keysyms[6].content = ResolveKeysym(cell.keycode_raw, Mod5Mask | Mod3Mask);
	cell.keysyms[7].content = ResolveKeysym(cell.keycode_raw, Mod5Mask | Mod3Mask | ShiftMask);
}

std::u32string XLib::ResolveKeysym(KeyCode code, uint state) {
	static char buf[64];
	::memset(buf, 0, sizeof(buf));
	KeySym sym;

	XKeyPressedEvent ev;
	ev.type	   = KeyPress;
	ev.display = display;
	ev.keycode = code;
	ev.state   = state;

	XLookupString(&ev, buf, 64, &sym, nullptr);
	return sym == NoSymbol ? U"" : ToUTF32(buf);
}
