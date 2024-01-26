cmake_minimum_required(VERSION 3.6)
project(wormwood C)

# Include pkg-config module.
include(FindPkgConfig)

# Find ncurses pkg.
pkg_check_modules(NCURSES REQUIRED ncurses)

# Add our executable.
add_executable(wormwood "wormwood.c")

# Link our executable against libncurses.
target_link_libraries(wormwood ${NCURSES_LINK_LIBRARIES})
