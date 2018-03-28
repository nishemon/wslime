
include_directories(${FCITX4_FCITX_INCLUDE_DIRS})
include_directories(${FCITX4_FCITX_CONFIG_INCLUDE_DIRS})
include_directories(${FCITX4_FCITX_UTILS_INCLUDE_DIRS})

set(FCITX_WSLIME_SRC fcitx-wslime.c wslime-relay.c)

fcitx_add_addon_full(wslime
  SOURCES ${FCITX_WSLIME_SRC}
  IM_CONFIG wslime.conf
  LINK_LIBS ${CMAKE_THREAD_LIBS_INIT})
