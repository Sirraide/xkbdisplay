#include <xkb++/layout.hh>

auto ISO105Traits::KeyName(isz row, isz idx) -> std::string {
    switch (row) {
        default: Unreachable();
        case 0: {
            if (idx == 0) return "TLDE";
            return std::format("AE{:02}", idx);
        }

        case 1: return std::format("AD{:02}", idx + 1);
        case 2: {
            if (idx == 12) return "BKSL";
            return std::format("AC{:02}", idx + 1);
        }

        case 3: {
            if (idx == 0) return "LSGT";
            return std::format("AB{:02}", idx + 1);
        }
    }
}
