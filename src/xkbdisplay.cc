#include <chrono>
#include <clopts.hh>
#include <functional>
#include <map>
#include <print>
#include <ranges>
#include <thread>
#include <vector>

#include <base/Base.hh>
#include <base/Text.hh>

#include <X11/X.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <xkb++/layout.hh>
#include <xkb++/main.hh>

using namespace base;
using namespace layout;

// ============================================================================
//  Context and Data
// ============================================================================
constexpr usz FPS = 144;
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

    std::vector<Text> menu_text{};
    std::string font_name;
    std::map<std::pair<std::string, u32>, XftFont*> font_cache{};

    /// Cell borders are allocated separately so we can pass them to
    /// X11 in one go.
    Layout<Cell, ISO105Traits> cells{};
    Layout<XRectangle, ISO105Traits> cell_borders{};

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
    static auto Create(std::string font) -> Result<std::unique_ptr<DisplayContext>>;

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

bool IsDiacritic(c32 c) {
    switch (c.category()) {
        default: return false;
        case text::CharCategory::CombiningSpacingMark:
        case text::CharCategory::EnclosingMark:
        case text::CharCategory::NonSpacingMark:
        case text::CharCategory::ConnectorPunctuation:
            return true;
    }
}

// ============================================================================
//  Initialisation and Main Loop
// ============================================================================
DisplayContext::~DisplayContext() {
    if (display) XCloseDisplay(display);
}

auto DisplayContext::Create(std::string font) -> Result<std::unique_ptr<DisplayContext>> {
    std::unique_ptr<DisplayContext> C{new DisplayContext()};
    C->font_name = std::move(font);
    Try(C->InitDisplay());
    Try(C->InitFonts());
    C->InitCells();
    return C;
}

void DisplayContext::InitCells() { // clang-format off
    for (auto [cell, border, keycode, label] : vws::zip(
        cells.keys(),
        cell_borders.keys(),
        ISO105Traits::KeyCodes,
        ISO105Traits::KeyLabels
    )) {
        cell.label_char = label;
        cell.border = &border;
        cell.keycode_raw = keycode;
        cell.keycode.content = text::ToUTF32(std::to_string(keycode));
        ResolveKeysym(cell.keysyms[0], keycode, 0);
        ResolveKeysym(cell.keysyms[1], keycode, ShiftMask);
        ResolveKeysym(cell.keysyms[2], keycode, Mod5Mask);
        ResolveKeysym(cell.keysyms[3], keycode, ShiftMask | Mod5Mask);
        ResolveKeysym(cell.keysyms[4], keycode, Mod3Mask);
        ResolveKeysym(cell.keysyms[5], keycode, ShiftMask | Mod3Mask);
        ResolveKeysym(cell.keysyms[6], keycode, Mod5Mask | Mod3Mask);
        ResolveKeysym(cell.keysyms[7], keycode, Mod5Mask | Mod3Mask | ShiftMask);
    }
} // clang-format on

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
    font = Font(font_name, font_sz);

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
    auto borders = cell_borders.keys();
    XDrawRectangles(display, window, gc, borders.data(), int(borders.size()));
    for (const auto& cell : cells.keys()) {
        DrawTextElem(cell.label, &xft_grey);
        DrawTextElem(cell.keycode, &xft_grey, Font(font_name, font_sz / 2));
        DrawTextElems(cell.keysyms, &xft_fgcolour, Font(font_name, u32(font_sz / 1.33)));
    }
    DrawTextElems(menu_text, &xft_fgcolour);
}

void DisplayContext::GenerateKeyboard() {
    const u16 cell_size = u16(RelativeToWidth(.0625));
    const u16 gap = u16(RelativeToWidth(.0125));
    const u16 top_offset = u16(RelativeToWidth(.05));
    const u16 right_margin = u16(w_width - cell_size);

    // Figure out where to place the cells and set up the borders around them.
    for (usz cell_index = 0, row = 0; row < ISO105Traits::Rows.size(); row++) { // clang-format off
        for (
            u16 x = u16(gap + row * 2 * gap), i = 0;
            x < right_margin and i < ISO105Traits::Rows[row];
            x += cell_size + gap, i++
        ) *cells.keys()[cell_index++].border = {
            .x = i16(x),
            .y = i16((row + 1) * gap + row * cell_size + top_offset),
            .width = cell_size,
            .height = cell_size,
        };
    } // clang-format on

    // Set up the cell labels, keycodes, and keysyms
    for (usz cell_index = 0, i = 0; i < ISO105Traits::KeyCount; i++) {
        auto& cell = cells.keys()[cell_index++];
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
    std::u32string f;
    f += U"Font: ";
    f += text::ToUTF32(font_name);
    f += U" ";
    f += text::ToUTF32(std::to_string(int(font_sz)));
    DrawCentredTextAt(f, int(w_width / 2u), HEIGHT_TIMES_TWO);
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
            text.content = text::ToUTF32(std::string_view{buf.data(), usz(size)});
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

auto Main(int argc, char** argv) -> Result<int> {
    using namespace command_line_options;
    using options = clopts< // clang-format off
        option<"-f", "The font to use">,
        help<>
    >; // clang-format on

    auto opts = options::parse(argc, argv);
    auto ctx = Try(DisplayContext::Create(opts.get<"-f">("Charis SIL")));
    ctx->Run();
    return 0;
}
