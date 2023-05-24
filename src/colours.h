/**
 * \file colours.h
 * \brief Colours codes for printing to the terminal
 */

#ifndef COLOURS_HPP
#define COLOURS_HPP

#include <string>

namespace Colour {
    const std::string GREEN{"\033[1;32m"};
    const std::string YELLOW{"\033[1;33m"};
    const std::string BLUE{"\033[1;34m"};
    const std::string CYAN{"\033[1;36m"};
    const std::string ORANGE{"\033[1;38;5;208m"};
    const std::string PINK{"\033[1;38;5;207m"};
    const std::string PALE{"\033[1;38;5;159m"};
    const std::string RESET{"\033[0m"};
}

#endif
