cmake_minimum_required(VERSION 3.6)
project(wormwood C)

# Include pkg-config module.
include(FindPkgConfig)

# Find ncurses pkg.
pkg_check_modules(NCURSES REQUIRED ncurses)

set(WW_HEADERS
    "common.h"
    "signal.h"
    "console_win.h"
    "reactor_mgr.h"
    "reactor.h"
    "status_win.h"
)

set(WW_SOURCES
    "common.c"
    "console_win.c"
    "signal.c"
    "reactor_mgr.c"
    "reactor.c"
    "status_win.c"
    "wormwood.c"
)

add_compile_options(-fno-pie -pthread)
add_link_options(-no-pie -pthread)

# Add our executable.
add_executable(wormwood ${WW_SOURCES} ${WW_HEADERS})

# Link our executable against libncurses.
target_link_libraries(wormwood ${NCURSES_LINK_LIBRARIES})
