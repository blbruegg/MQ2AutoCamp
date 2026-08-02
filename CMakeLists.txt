# MQ2AutoCamp
# Forked from RedGuides/MQ2AutoCamp (github.com/RedGuides org sweep, forum-plugins-2026-08-02
# follow-up batch). Vendored as a git submodule tracking https://github.com/blbruegg/MQ2AutoCamp
# (fork of RedGuides/MQ2AutoCamp). See F:\\EQProject\\docs\\re_progress.md for the write-up.

include("../../Plugin.cmake")

set(MQ2AutoCamp_SOURCES
    "MQ2AutoCamp.cpp"
    "MQ2AutoCamp.rc"
    "resource.h"
)

source_groups("Source Files" ${MQ2AutoCamp_SOURCES})

add_library(MQ2AutoCamp SHARED
    ${MQ2AutoCamp_SOURCES}
)

set_target_properties(MQ2AutoCamp PROPERTIES FOLDER "core/plugins")

target_Plugin_props(MQ2AutoCamp)

target_link_libraries(MQ2AutoCamp PRIVATE MQ2Main)
