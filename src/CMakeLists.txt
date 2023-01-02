

set(tistouchess_src
  main.cpp
  board.hpp
  board.cpp
  move.hpp
  types.hpp
  board_renderer.hpp
  board_renderer.cpp
  fen_reader.cpp
  fen_reader.hpp
  move_generation.hpp
  move_generation.cpp
  evaluation.cpp
  evaluation.hpp
  engine.hpp
  engine.cpp
  timer.hpp
  timer.cpp
  move_ordering.hpp
  move_ordering.cpp
  uci.hpp
  uci.cpp
  transposition_table.hpp
  transposition_table.cpp
)

#add_executable(chess WIN32 ${tistouchess_src})
add_executable(tistouchess  ${tistouchess_src})
target_link_libraries(tistouchess
  PRIVATE
  fmt::fmt-header-only
)

if (WITH_SDL)
  target_compile_options(tistouchess PRIVATE -DCHESS_ENABLE_SDL)
  target_link_libraries(tistouchess
    PUBLIC
    SDL2::SDL2
    SDL2_image::SDL2_image
  )
endif(WITH_SDL)

target_compile_features(tistouchess PRIVATE cxx_std_20)


if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND WITH_SDL)
  # copy dll from libraries into destination folder:
  add_custom_command(TARGET tistouchess
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E
    copy_if_different
    $<TARGET_FILE:SDL2::SDL2>
    $<TARGET_FILE:SDL2_image::SDL2_image>
    $<TARGET_FILE_DIR:tistouchess>)
endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND WITH_SDL)
