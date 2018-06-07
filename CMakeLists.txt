cmake_minimum_required(VERSION 3.0)

add_executable(swe1r-patcher main.c)

if (WIN32)
  add_executable(swe1r-loader main.c)
  target_compile_definitions(swe1r-loader PUBLIC -DLOADER=1)
endif()

# font0
configure_file(font0_0_test.data font0_0_test.data COPYONLY)

# font1
configure_file(font1_0_test.data font1_0_test.data COPYONLY)
configure_file(font1_1_test.data font1_1_test.data COPYONLY)
configure_file(font1_2_test.data font1_2_test.data COPYONLY)

# font2 (font2_0 is the same as font1_2)
configure_file(font1_2_test.data font2_0_test.data COPYONLY)

# font3 (font3_0 is the same as font1_2)
configure_file(font1_2_test.data font3_0_test.data COPYONLY)

#FIXME: font4


