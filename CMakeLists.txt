cmake_minimum_required(VERSION 3.0)

add_executable(swe1r-patcher main.c)

if (WIN32)
  add_executable(swe1r-loader main.c)
  target_compile_definitions(swe1r-loader PUBLIC -DLOADER=1)

  add_library(dinput SHARED main.c dinput.def)
  set_target_properties(dinput PROPERTIES PREFIX "")
  target_compile_definitions(dinput PUBLIC -DLOADER=1 -DDLL=1)
endif()

# font0
configure_file(textures/font0_0_test.data textures/font0_0_test.data COPYONLY)

# font1
configure_file(textures/font1_0_test.data textures/font1_0_test.data COPYONLY)
configure_file(textures/font1_1_test.data textures/font1_1_test.data COPYONLY)
configure_file(textures/font1_2_test.data textures/font1_2_test.data COPYONLY)

# font2 (font2_0 is the same as font1_2)
configure_file(textures/font1_2_test.data textures/font2_0_test.data COPYONLY)

# font3 (font3_0 is the same as font1_2)
configure_file(textures/font1_2_test.data textures/font3_0_test.data COPYONLY)

# font4
configure_file(textures/font4_0_test.data textures/font4_0_test.data COPYONLY)

# README.md
configure_file(README.md README.md COPYONLY)

