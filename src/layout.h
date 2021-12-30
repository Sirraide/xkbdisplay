#ifndef XKBDISPLAY_LAYOUT_H
#define XKBDISPLAY_LAYOUT_H

#include <string>
#include "utils.h"
#include "x.h"


struct Layout {
	std::string filename;
	xkb_keymap* map;
	static xkb_context* ctx;
	XLib* X;

	Layout(XLib* _X);
	Layout(const Layout& file_path) = delete;


};

#endif // XKBDISPLAY_LAYOUT_H
