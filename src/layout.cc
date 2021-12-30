#include "layout.h"

xkb_context* Layout::ctx;

void InitXKB() {
	if (!Layout::ctx)
		Layout::ctx = Verify(xkb_context_new(XKB_CONTEXT_NO_FLAGS), "xkb_context_new()");
}

Layout::Layout(XLib* _X) : X(_X) {
	InitXKB();
	int device = xkb_x11_get_core_keyboard_device_id(X->connexion);
	if (device == -1) Die("xkb_x11_get_core_keyboard_device_id()");
	map = Verify(xkb_x11_keymap_new_from_device(ctx, X->connexion, device,
		XKB_KEYMAP_COMPILE_NO_FLAGS), "xkb_x11_keymap_new_from_device()");
}
