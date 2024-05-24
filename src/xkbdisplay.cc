#include <chrono>
#include <functional>
#include <map>
#include <print>
#include <thread>
#include <unistd.h>
#include <vector>
#include <X11/X.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <xkb++/utils.hh>
#include <xkb++/xkbdisplay.hh>

// ============================================================================
//  Context and Data
// ============================================================================
constexpr usz FPS = 144;
constexpr usz KEYS_IN_ROW_1 = 13;
constexpr usz KEYS_IN_ROW_2 = 12;
constexpr usz KEYS_IN_ROW_3 = 12;
constexpr usz KEYS_IN_ROW_4 = 11;
constexpr usz KEY_COUNT = KEYS_IN_ROW_1 + KEYS_IN_ROW_2 + KEYS_IN_ROW_3 + KEYS_IN_ROW_4;
constexpr int HEIGHT_TIMES_TWO = -1;

struct Text {
    int x{};
    int y{};
    std::u32string content;
    bool diacritic = false;
};

struct Cell {
    char32_t label_char{};
    KeyCode keycode_raw;
    Text keycode{};
    XRectangle* border{};
    Text label{};
    std::array<Text, 8> keysyms{};
};

class DisplayContext {
    static constexpr u64 bgcolour = 0x2D'2A2E;
    static constexpr u64 fgcolour = 0xFC'FCFA;
    static constexpr u64 grey = 0x5B'595C;
    static constexpr u32 base_width = 1'400;
    static constexpr u32 base_height = 550;
    static constexpr u32 base_line_width = 2;

    Display* display{};
    Window window{};
    Atom delete_window{};
    GC gc{};
    int screen{};
    XWindowAttributes attrs{};
    XftFont* font{};
    XftDraw* draw{};
    XRenderColor x_fgcolour{};
    XRenderColor x_grey{};
    XRenderColor x_red{};
    XftColor xft_fgcolour{};
    XftColor xft_grey{};
    XftColor xft_red{};

    std::array<Cell, KEY_COUNT> cells{};
    std::array<XRectangle, KEY_COUNT> cell_borders{};
    std::vector<Text> menu_text{};
    std::map<std::pair<std::string, u32>, XftFont*> font_cache{};

    u32 w_width = 1'400;
    u32 w_height = 550;
    u32 font_sz;

    u64 white{};
    u64 black{};

public:
    DisplayContext(const DisplayContext&) = delete;
    DisplayContext(DisplayContext&&) = delete;
    DisplayContext& operator=(const DisplayContext&) = delete;
    DisplayContext& operator=(DisplayContext&&) = delete;
    ~DisplayContext();

    /// Run the display loop.
    void Run();

    /// Create a new display context.
    static auto Create() -> Result<std::unique_ptr<DisplayContext>>;

private:
    DisplayContext() = default;

    void DrawCells();
    void DrawCentredTextAt(const std::u32string& text, int xpos, int ypos);
    void DrawTextAt(int x, int y, std::u32string text);
    void DrawTextElem(const Text& elem, const XftColor* colour, XftFont* fnt = nullptr) const;
    void DrawTextElems(const auto& text_elems, XftColor* colour, XftFont* fnt = nullptr) const;
    auto Font(const std::string& name, u32 font_sz) -> XftFont*;
    void GenerateKeyboard();
    void GenerateMenuText();
    void InitCells();
    auto InitDisplay() -> Result<>;
    auto InitFonts() -> Result<>;
    void Redraw();
    auto RelativeToWidth(double f) const -> u32 { return u32(w_width * f); }
    auto RelativeToHeight(double f) const -> u32 { return u32(w_height * f); }
    void ResolveKeysym(Text& text, KeyCode code, u32 state) const;
    auto TextExtents(std::u32string_view t, XftFont* fnt = nullptr) const -> XGlyphInfo;
};

// ============================================================================
//  Helpers
// ============================================================================
int HandleError(Display* display, XErrorEvent* e) {
    static char buffer[2'048];
    XGetErrorText(display, e->error_code, buffer, 2'048);
    std::println(stderr, "Fatal Error: {}", buffer);
    std::exit(1);
}

constexpr auto U16Colour(u16 colour) -> u16 { return colour * 256; }
constexpr auto XColour(u32 colour) -> XRenderColor {
    return {
        .red = U16Colour(colour >> 16 & 0xFF),
        .green = U16Colour(colour >> 8 & 0xFF),
        .blue = U16Colour(colour & 0xFF),
        .alpha = U16Colour(0xFF),
    };
}

// ============================================================================
//  Initialisation and Main Loop
// ============================================================================
auto xkbdisplay::Main(int, char**) -> Result<int> {
    auto ctx = Try(DisplayContext::Create());
    ctx->Run();
    return 0;
}

DisplayContext::~DisplayContext() {
    if (display) XCloseDisplay(display);
}

auto DisplayContext::Create() -> Result<std::unique_ptr<DisplayContext>> {
    std::unique_ptr<DisplayContext> C{new DisplayContext()};
    Try(C->InitDisplay());
    Try(C->InitFonts());
    C->InitCells();
    return C;
}

void DisplayContext::InitCells() { // clang-format off
    static constexpr auto label_chars = U"¬1234567890-=QWERTYUIOP[]ASDFGHJKL;'^´ZXCVBNM,./"sv; // +1 because \0
    static constexpr KeyCode keycodes[KEY_COUNT] = {
        49, 10, 11, 12, 13, 14, 15, 16, 17, 18,
        19, 20, 21, 24, 25, 26, 27, 28, 29, 30,
        31, 32, 33, 34, 35, 38, 39, 40, 41, 42,
        43, 44, 45, 46, 47, 48, 51, 94, 52, 53,
        54, 55, 56, 57, 58, 59, 60, 61
    }; // clang-format on

    for (size_t i = 0; i < KEY_COUNT; i++) {
        cells[i].label_char = label_chars[i];
        cells[i].border = cell_borders.data() + i;
        cells[i].keycode_raw = keycodes[i];
        cells[i].keycode.content = ToUtf32(std::to_string(keycodes[i]));
        for (size_t j = 0; j < 8; j++) {
            ResolveKeysym(cells[i].keysyms[0], keycodes[i], 0);
            ResolveKeysym(cells[i].keysyms[1], keycodes[i], ShiftMask);
            ResolveKeysym(cells[i].keysyms[2], keycodes[i], Mod5Mask);
            ResolveKeysym(cells[i].keysyms[3], keycodes[i], ShiftMask | Mod5Mask);
            ResolveKeysym(cells[i].keysyms[4], keycodes[i], Mod3Mask);
            ResolveKeysym(cells[i].keysyms[5], keycodes[i], ShiftMask | Mod3Mask);
            ResolveKeysym(cells[i].keysyms[6], keycodes[i], Mod5Mask | Mod3Mask);
            ResolveKeysym(cells[i].keysyms[7], keycodes[i], Mod5Mask | Mod3Mask | ShiftMask);
        }
    }
}

auto DisplayContext::InitDisplay() -> Result<> {
    display = XOpenDisplay(nullptr);
    if (not display) return Error("Failed to open display");

    // Set up error handler.
    XSetErrorHandler(HandleError);

    screen = XDefaultScreen(display);
    window = XCreateSimpleWindow(display, XRootWindow(display, screen), 0, 0, w_width, w_height, 0, 0, 0x2D'2A2E);
    white = XWhitePixel(display, screen);
    black = XBlackPixel(display, screen);

    XGetWindowAttributes(display, window, &attrs);

    // Enable receiving of WM_DELETE_WINDOW.
    delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &delete_window, 1);

    // Allocate the GC.
    XGCValues values{};
    values.cap_style = CapRound;
    values.join_style = JoinRound;
    u64 value_mask = GCCapStyle | GCJoinStyle;

    gc = XCreateGC(display, window, value_mask, &values);
    if (not gc) return Error("Failed to create graphics context");
    return {};
}

auto DisplayContext::InitFonts() -> Result<> {
    draw = XftDrawCreate(display, window, attrs.visual, attrs.colormap);
    if (not draw) return Error("Failed to create XftDraw");
    x_fgcolour = XColour(fgcolour);
    x_grey = XColour(grey);
    x_red = XColour(0xFF'6188);
    XftColorAllocValue(display, attrs.visual, attrs.colormap, &x_fgcolour, &xft_fgcolour);
    XftColorAllocValue(display, attrs.visual, attrs.colormap, &x_grey, &xft_grey);
    XftColorAllocValue(display, attrs.visual, attrs.colormap, &x_red, &xft_red);
    return {};
}

void DisplayContext::Redraw() {
    XGetWindowAttributes(display, window, &attrs);
    w_width = u32(attrs.width);
    w_height = u32(attrs.height);
    XSetLineAttributes(
        display,
        gc,
        u32(base_line_width * double(w_width) / double(base_width)),
        LineSolid,
        CapRound,
        JoinRound
    );

    font_sz = RelativeToWidth(.015);
    font = Font("Charis SIL", font_sz);

    GenerateKeyboard();
    GenerateMenuText();
    XClearWindow(display, window);
    DrawCells();
}

void DisplayContext::Run() {
    // Display the window.
    XMapWindow(display, window);
    XSync(display, False);

    // Set a min size.
    auto* hints = XAllocSizeHints();
    hints->min_height = static_cast<int>(base_height / 2);
    hints->min_width = static_cast<int>(base_width / 2);
    hints->flags = PMinSize | PAspect;
    hints->min_aspect = hints->max_aspect = {.x = base_width, .y = base_height};
    XSetWMNormalHints(display, window, hints);

    // Set the window name.
    XStoreName(display, window, "XKBDisplay");

    // Set line params.
    XSetForeground(display, gc, fgcolour);
    XSetBackground(display, gc, bgcolour);
    XSetFillStyle(display, gc, FillSolid);
    XSetLineAttributes(display, gc, base_line_width, LineSolid, CapRound, JoinRound);

    // Subscribe to events.
    XSelectInput(display, window, StructureNotifyMask);

    // Initialise the window content.
    Redraw();

    bool quit = false;
    while (not quit) {
        while (XPending(display)) {
            XEvent e{};
            XNextEvent(display, &e);
            switch (e.type) {
                default: continue;
                case ConfigureNotify: Redraw(); break;
                case ClientMessage:
                    if (Atom(e.xclient.data.l[0]) == delete_window) quit = true;
                    break;
            }
        }

        std::this_thread::sleep_for(1'000'000us / FPS);
    }
}

// ============================================================================
//  Character Handling
// ============================================================================
void DisplayContext::DrawCells() {
    XDrawRectangles(display, window, gc, cell_borders.data(), int(cell_borders.size()));
    for (const auto& cell : cells) {
        DrawTextElem(cell.label, &xft_grey);
        DrawTextElem(cell.keycode, &xft_grey, Font("Charis SIL", font_sz / 2));
        DrawTextElems(cell.keysyms, &xft_fgcolour, Font("Charis SIL", u32(font_sz / 1.33)));
    }
    DrawTextElems(menu_text, &xft_fgcolour);
}

void DisplayContext::GenerateKeyboard() {
    const u16 cell_size = u16(RelativeToWidth(.0625));
    const u16 gap = u16(RelativeToWidth(.0125));
    const u16 top_offset = u16(RelativeToWidth(.05));
    const u16 right_margin = u16(w_width - cell_size);

    // Figure out where to place the cells and set up the borders around them.
    auto ComputeBorders = [&, cell_index = usz(0), n = u16(0)](u16 num_keys) mutable { // clang-format off
        for (
            u16 x = u16(gap + n * 2 * gap), i = 0;
            x + cell_size < right_margin and i < num_keys;
            x += cell_size + gap, i++
        ) *cells[cell_index++].border = {
            .x = i16(x),
            .y = i16((n + 1) * gap + n * cell_size + top_offset),
            .width = cell_size,
            .height = cell_size,
        };
        n++;
    }; // clang-format on

    ComputeBorders(KEYS_IN_ROW_1);
    ComputeBorders(KEYS_IN_ROW_2);
    ComputeBorders(KEYS_IN_ROW_3);
    ComputeBorders(KEYS_IN_ROW_4);

    // Set up the cell labels, keycodes, and keysyms
    for (usz cell_index = 0, i = 0; i < KEY_COUNT; i++) {
        auto& cell = cells[cell_index++];
        auto* rect = cell.border;

        // Compute text extents.
        std::u32string text{cell.label_char};
        auto extents = TextExtents(text);

        // Map extents relative to the cell position.
        auto xpos = rect->x + rect->width / 2 - extents.width / 2;
        auto ypos = rect->y + rect->height / 2 + extents.height / 2;
        if (cell.label_char == U'Q') ypos -= font->descent / 2;
        cell.label = {.x = xpos, .y = ypos, .content = text};
        cell.keycode.x = xpos;
        cell.keycode.y = int(cell.border->y) + int(font_sz) / 2 + 4;

        // Er, this works, I’m not going to question how...
        const auto vstride = int(.23 * rect->width);
        const auto hstride = int(.25 * rect->height);
        const int v_start = rect->x + int(.1 * rect->width);
        cell.keysyms[0].x = cell.keysyms[1].x = v_start;
        cell.keysyms[2].x = cell.keysyms[3].x = v_start + vstride;
        cell.keysyms[4].x = cell.keysyms[5].x = v_start + vstride * 2;
        cell.keysyms[6].x = cell.keysyms[7].x = v_start + vstride * 3;
        cell.keysyms[0].y = cell.keysyms[2].y =
            cell.keysyms[4].y = cell.keysyms[6].y = rect->y + rect->height - hstride;
        cell.keysyms[1].y = cell.keysyms[3].y =
            cell.keysyms[5].y = cell.keysyms[7].y = rect->y + hstride;
    }
}

void DisplayContext::GenerateMenuText() {
    menu_text.clear();
    std::u32string font_name = U"Font: Charis SIL " + ToUtf32(std::to_string(int(font_sz)));
    DrawCentredTextAt(font_name, w_width / 2, HEIGHT_TIMES_TWO);
}

void DisplayContext::ResolveKeysym(Text& text, KeyCode code, u32 state) const {
    std::array<char, 64> buf{};
    KeySym sym;

    // Synthesise an event as though the user had pressed the key in question,
    // with the given modifiers, and ask X what symbol that key press produces.
    XKeyPressedEvent ev;
    ev.type = KeyPress;
    ev.display = display;
    ev.keycode = code;
    ev.state = state;
    auto size = XLookupString(&ev, buf.data(), buf.size(), &sym, nullptr);

    if (sym == NoSymbol) text.content = U"";
    else {
        // I’m candidly not sure what in here is supposed to be able to throw,
        // but I recall having to add that for some reason, so I’m not removing
        // it now...
        try {
            text.content = ToUtf32(std::string_view{buf.data(), usz(size)});
            text.diacritic = std::string_view(XKeysymToString(sym)).starts_with("dead") || IsDiacritic(text.content[0]);
            if (text.diacritic) text.content = U"◌" + text.content;
        } catch (...) {
            text.content = U"";
        }
    }
}

auto DisplayContext::TextExtents(std::u32string_view t, XftFont* fnt) const -> XGlyphInfo {
    if (fnt == nullptr) fnt = font;
    XGlyphInfo extents;
    XftTextExtents32(
        display,
        fnt,
        reinterpret_cast<const FcChar32*>(t.data()),
        int(t.size()),
        &extents
    );
    return extents;
}

// ============================================================================
//  Drawing
// ============================================================================
void DisplayContext::DrawCentredTextAt(const std::u32string& text, int xpos, int ypos) {
    auto extents = TextExtents(text);
    auto x = xpos - extents.width / 2;
    auto y = ypos == HEIGHT_TIMES_TWO ? extents.height * 2 : ypos;
    DrawTextAt(int(x), int(y), text);
}

void DisplayContext::DrawTextAt(int x, int y, std::u32string text) {
    menu_text.push_back({x, y, std::move(text)});
}

void DisplayContext::DrawTextElems(const auto& text_elems, XftColor* colour, XftFont* fnt) const {
    for (auto& text_elem : text_elems) DrawTextElem(text_elem, colour, fnt);
}

void DisplayContext::DrawTextElem(const Text& elem, const XftColor* colour, XftFont* fnt) const {
    if (elem.content.empty()) return;
    if (fnt == nullptr) fnt = font;
    if (elem.diacritic) colour = &xft_red;
    XftDrawString32(
        draw,
        colour,
        fnt,
        elem.x,
        elem.y,
        reinterpret_cast<const FcChar32*>(elem.content.data()),
        int(elem.content.size())
    );
}

auto DisplayContext::Font(const std::string& name, u32 font_sz) -> XftFont* {
    std::pair pair = {name, font_sz};
    if (font_cache.contains(pair)) return font_cache[pair];
    auto f = font_cache[std::move(pair)] = XftFontOpen(
        display,
        screen,
        XFT_FAMILY,
        XftTypeString,
        name.data(),
        XFT_SIZE,
        XftTypeDouble,
        double(font_sz),
        nullptr
    );

    /// We’re not really equipped to deal with failure here...
    Assert(f, "XftFontOpen(): Could not open {}:{}", name, font_sz);
    return f;
}

//////////////////////
//////////////////////
//////////////////////
//////////////////////
//////////////////////
