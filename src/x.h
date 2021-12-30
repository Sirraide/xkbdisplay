#ifndef XKBDISPLAY_X_H
#define XKBDISPLAY_X_H

#include "utils.h"

#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <functional>
#include <map>
#include <vector>

struct Text {
	int			   x{};
	int			   y{};
	std::u32string content;
};

struct XLib {
	static constexpr ulong bgcolour = 0x2D2A2E;
	static constexpr ulong fgcolour = 0xFCFCFA;

	static constexpr uint base_width	  = 1450;
	static constexpr uint base_height	  = 500;
	static constexpr uint base_line_width = 4;

	Display*		  display{};
	Window			  window{};
	Atom			  delete_window{};
	GC				  gc{};
	int				  screen{};
	XWindowAttributes attrs{};
	XftFont*		  font{};
	XftDraw*		  draw{};
	XRenderColor	  x_colour{};
	XftColor		  xft_colour{};

	uint   w_width	= 1450;
	uint   w_height = 500;
	double font_sz;

	ulong white{};
	ulong black{};

	void	   Draw();
	void	   DrawTextAt(int x, int y, std::u32string text);
	void	   DrawTextElems();
	void	   GenerateGrid();
	static int HandleError(Display* display, XErrorEvent* e);
	void	   Redraw();
	void	   Run(std::function<void(XEvent& e)> event_callback);

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

	std::vector<XRectangle>	   rects{};
	std::vector<Text>		   text_elems{};
	std::map<double, XftFont*> main_font_cache{};

	XLib();
	~XLib();
};

#endif // XKBDISPLAY_X_H
