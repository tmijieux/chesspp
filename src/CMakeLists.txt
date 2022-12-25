

set(chess_src
  main.cpp
  board.hpp
  board.cpp
  move.hpp
  types.hpp
  board_renderer.hpp
  board_renderer.cpp
  FenReader.cpp
  FenReader.hpp
  move_generation.hpp
  move_generation.cpp
  evaluation.cpp
  evaluation.hpp
  engine.hpp
  engine.cpp
)
#add_executable(chess WIN32 ${chess_src})
add_executable(chess  ${chess_src})
target_link_libraries(chess PUBLIC SDL2::SDL2 SDL2_image::SDL2_image)
target_compile_features(chess PRIVATE cxx_std_20)


if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # copy dll from libraries into destination folder:
  add_custom_command(TARGET chess
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E
    copy_if_different
    $<TARGET_FILE:SDL2::SDL2>
    $<TARGET_FILE:SDL2_image::SDL2_image>
    $<TARGET_FILE_DIR:chess>)
endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
