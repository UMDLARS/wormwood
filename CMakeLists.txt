cmake_minimum_required(VERSION 3.6)
project(wormwood C)

# Include pkg-config module.
include(FindPkgConfig)

# Find ncurses pkg.
pkg_check_modules(NCURSES REQUIRED ncurses)

set(WW_HEADERS
    "common.h"
    "console_win.h"
    "status_win.h"
)

set(WW_SOURCES
    "common.c"
    "console_win.c"
    "status_win.c"
    "wormwood.c"
)

# Add our executable.
add_executable(wormwood ${WW_SOURCES} ${WW_HEADERS})

# Link our executable against libncurses.
target_link_libraries(wormwood ${NCURSES_LINK_LIBRARIES})
