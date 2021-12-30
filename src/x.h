#ifndef XKBDISPLAY_X_H
#define XKBDISPLAY_X_H

#include "utils.h"

#include <X11/X.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <functional>
#include <map>
#include <vector>
#include <xcb/xcb.h>
#include <xkbcommon/xkbcommon-x11.h>

#define KEYS_IN_ROW_1 13
#define KEYS_IN_ROW_2 12
#define KEYS_IN_ROW_3 12
#define KEYS_IN_ROW_4 11

#define KEY_COUNT (KEYS_IN_ROW_1 + KEYS_IN_ROW_2 + KEYS_IN_ROW_3 + KEYS_IN_ROW_4)

struct Text {
	int			   x{};
	int			   y{};
	std::u32string content;
};

enum Alignment : int {
	HEIGHT_TIMES_TWO = -1
};

struct Cell {
	char32_t			label_char{};
	KeyCode				keycode_raw;
	Text				keycode{};
	XRectangle*			border{};
	Text				label{};
	std::array<Text, 8> keysyms{};
};

struct XLib {
	static constexpr ulong bgcolour = 0x2D2A2E;
	static constexpr ulong fgcolour = 0xFCFCFA;
	static constexpr ulong grey		= 0x5B595C;

	static constexpr uint base_width	  = 1400;
	static constexpr uint base_height	  = 550;
	static constexpr uint base_line_width = 3;

	Display*		  display{};
	xcb_connection_t* connexion{};
	Window			  window{};
	Atom			  delete_window{};
	GC				  gc{};
	int				  screen{};
	XWindowAttributes attrs{};
	XftFont*		  font{};
	XftDraw*		  draw{};
	XRenderColor	  x_fgcolour{};
	XftColor		  xft_fgcolour{};
	XRenderColor	  x_grey{};
	XftColor		  xft_grey{};

	uint   w_width	= 1400;
	uint   w_height = 550;
	ushort font_sz;

	ulong white{};
	ulong black{};

	void	   DrawCells();
	void	   DrawCentredTextAt(const std::u32string& text, int xpos, int ypos);
	void	   DrawTextAt(int x, int y, std::u32string text);
	void	   DrawTextElems(const std::vector<Text>& text_elems, XftColor* colour, XftFont* fnt = nullptr) const;
	void	   DrawTextElem(const Text& elem, XftColor* colour, XftFont* fnt = nullptr) const;
	XftFont*   Font(const std::string& name, ushort font_sz);
	void	   GenerateKeyboard();
	void	   GenerateMenuText();
	static int HandleError(Display* display, XErrorEvent* e);
	void	   InitCells();
	void	   Redraw();
	void	   Run(std::function<void(XEvent& e)> event_callback);
	XGlyphInfo TextExtents(const std::u32string& t, XftFont* fnt = nullptr) const;

	inline constexpr ushort		   w(double f) const { return ushort(w_width * f); }
	inline constexpr ushort		   h(double f) const { return ushort(w_height * f); }
	static inline constexpr ushort U16Colour(ushort colour) {
		return colour * 256;
	}
	static inline constexpr XRenderColor XColour(uint colour) {
		return {
			.red   = U16Colour((colour >> 16) & 0xFF),
			.green = U16Colour((colour >> 8) & 0xFF),
			.blue  = U16Colour(colour & 0xFF),
			.alpha = U16Colour(0xFF),
		};
	}

	std::array<Cell, KEY_COUNT>						   cells{};
	std::array<XRectangle, KEY_COUNT>				   cell_borders{};
	std::vector<Text>								   menu_text{};
	std::map<std::pair<std::string, ushort>, XftFont*> font_cache{};
	XLib();
	~XLib();
};

#endif // XKBDISPLAY_X_H
