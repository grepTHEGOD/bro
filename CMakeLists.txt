cmake_minimum_required(VERSION 3.10)

project(bro VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror=implicit-function-declaration")

include(GNUInstallDirs)

set(BRO_SOURCES
    src/main.c
    src/util.c
    src/repo.c
    src/github.c
    src/remote.c
    src/refs.c
    src/index.c
    src/vim_ide.c
    src/milanote_editor.c
    src/thunderbird_email.c
    src/core/blob.c
    src/core/commit.c
    src/core/object.c
    src/core/odb.c
    src/core/sha1.c
    src/core/tree.c
    src/core/zlib_utils.c
    src/commands/init.c
    src/commands/slap.c
    src/commands/yeet.c
    src/commands/vibe-check.c
    src/commands/yap.c
    src/commands/squad.c
    src/commands/new-squad.c
    src/commands/teleport.c
    src/commands/launch.c
    src/commands/absorb.c
    src/commands/yoink.c
    src/commands/steal.c
    src/commands/fuse.c
    src/commands/rewind.c
    src/commands/yoink-cherry.c
    src/commands/undo-vibes.c
    src/commands/soft-undo.c
    src/commands/hard-undo.c
    src/commands/oopsie.c
    src/commands/panic-revert.c
    src/commands/amend-yeet.c
    src/commands/delete-bro.c
    src/commands/yeet-file.c
    src/commands/untrack-bro.c
    src/commands/scrub.c
    src/commands/hide.c
    src/commands/roast.c
    src/commands/diff-bro.c
    src/commands/peek.c
    src/commands/expose.c
    src/commands/hash-slap.c
    src/commands/ls-bro.c
    src/commands/drip.c
    src/commands/nickname.c
    src/commands/detective.c
    src/commands/snitch.c
    src/commands/search-bro.c
    src/commands/grep-it.c
    src/commands/blame-grep.c
    src/commands/bro-doctor.c
    src/commands/summary.c
    src/commands/who-dis.c
    src/commands/reflog-yap.c
    src/commands/time-machine.c
    src/commands/patch-mail.c
    src/commands/apply-mail.c
    src/commands/sticky-note.c
    src/commands/sidekick.c
    src/commands/multiverse.c
    src/commands/zip-pack.c
    src/commands/range-roast.c
    src/commands/archive-vibes.c
    src/commands/clean-house.c
    src/commands/ignore-bro.c
    src/commands/config-vibe.c
    src/commands/tui.c
    src/commands/live.c
    src/commands/force-launch.c
)

add_executable(bro ${BRO_SOURCES})

find_library(CURSES_LIBRARY NAMES ncursesw ncurses curses)
if(CURSES_LIBRARY)
    target_link_libraries(bro ${CURSES_LIBRARY})
else()
    target_link_libraries(bro curses)
endif()

find_library(JSON_C_LIBRARY NAMES json-c)
if(JSON_C_LIBRARY)
    target_link_libraries(bro ${JSON_C_LIBRARY})
endif()

find_library(ZLIB_LIBRARY NAMES z)
if(ZLIB_LIBRARY)
    target_link_libraries(bro ${ZLIB_LIBRARY})
else()
    target_link_libraries(bro z)
endif()

target_include_directories(bro PRIVATE ${CMAKE_SOURCE_DIR}/src ${CURSES_INCLUDE_DIR})

install(TARGETS bro RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
