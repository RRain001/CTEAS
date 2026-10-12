cmake_minimum_required(VERSION 3.20)
project(cteas_p6d_fortran_rebuild LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
enable_testing()

add_executable(cteas_p6d_source_wind1_real4 src/wind1_real4_main.cpp)
target_include_directories(cteas_p6d_source_wind1_real4 PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MSVC)
  target_compile_options(cteas_p6d_source_wind1_real4 PRIVATE /W4 /WX /fp:strict)
else()
  # Keep source REAL*4 roundings, no reassociation or FMA contraction.
  # Contrast intentional REAL*8 functions only in later stage ports.
  target_compile_options(cteas_p6d_source_wind1_real4 PRIVATE
    -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -fno-fast-math)
endif()

add_executable(cteas_p6d_source_similt_real4 src/similt_real4_main.cpp)
target_include_directories(cteas_p6d_source_similt_real4 PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MSVC)
  target_compile_options(cteas_p6d_source_similt_real4 PRIVATE /W4 /WX /fp:strict)
else()
  target_compile_options(cteas_p6d_source_similt_real4 PRIVATE
    -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -fno-fast-math)
endif()

set(P6D_VENDOR
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../../engine/vendor/src/CALMET_v6.5.0_L150223")
set(P6D_ORIGINAL_TEST
  "${CMAKE_CURRENT_SOURCE_DIR}/../reference/compare_original_wind1_firstguess.py")
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(P6D_GFORTRAN NAMES gfortran-11 gfortran-12 gfortran)
if(NOT P6D_GFORTRAN)
  message(FATAL_ERROR "Independent P6D source oracle requires gfortran")
endif()
if(NOT EXISTS "${P6D_VENDOR}/calmet.for")
  message(FATAL_ERROR "Missing immutable original CALMET Fortran source")
endif()
add_test(NAME p6d_rebuild_independent_wind1_source_real4
  COMMAND ${Python3_EXECUTABLE} "${P6D_ORIGINAL_TEST}"
    --vendor "${P6D_VENDOR}"
    --cpp "$<TARGET_FILE:cteas_p6d_source_wind1_real4>"
    --out "${CMAKE_CURRENT_BINARY_DIR}/oracle_wind1_real4")
set_tests_properties(p6d_rebuild_independent_wind1_source_real4
  PROPERTIES TIMEOUT 300)

add_test(NAME p6d_rebuild_independent_similt_psiud_original_real4_real8
  COMMAND ${Python3_EXECUTABLE}
    "${CMAKE_CURRENT_SOURCE_DIR}/../reference/compare_original_similt_profile.py"
    --vendor "${P6D_VENDOR}"
    --cpp "$<TARGET_FILE:cteas_p6d_source_similt_real4>"
    --out "${CMAKE_CURRENT_BINARY_DIR}/oracle_similt_real4")
set_tests_properties(p6d_rebuild_independent_similt_psiud_original_real4_real8
  PROPERTIES TIMEOUT 300)

add_executable(cteas_p6d_source_diagno_tail_real4 src/diagno_tail_real4_main.cpp)
target_include_directories(cteas_p6d_source_diagno_tail_real4 PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MSVC)
  target_compile_options(cteas_p6d_source_diagno_tail_real4 PRIVATE /W4 /WX /fp:strict)
else()
  target_compile_options(cteas_p6d_source_diagno_tail_real4 PRIVATE
    -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -fno-fast-math)
endif()
add_test(NAME p6d_rebuild_independent_diagno_final_real4
  COMMAND ${Python3_EXECUTABLE}
    "${CMAKE_CURRENT_SOURCE_DIR}/../reference/compare_original_diagno_final_segment.py"
    --vendor "${P6D_VENDOR}"
    --cpp "$<TARGET_FILE:cteas_p6d_source_diagno_tail_real4>"
    --out "${CMAKE_CURRENT_BINARY_DIR}/oracle_diagno_tail_real4")
set_tests_properties(p6d_rebuild_independent_diagno_final_real4
  PROPERTIES TIMEOUT 300)

add_executable(cteas_p6d_source_inter2_real4 src/inter2_real4_main.cpp)
target_include_directories(cteas_p6d_source_inter2_real4 PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MSVC)
  target_compile_options(cteas_p6d_source_inter2_real4 PRIVATE /W4 /WX /fp:strict)
else()
  target_compile_options(cteas_p6d_source_inter2_real4 PRIVATE
    -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -fno-fast-math)
endif()
add_test(NAME p6d_rebuild_independent_inter2_real4_real8_source
  COMMAND ${Python3_EXECUTABLE}
    "${CMAKE_CURRENT_SOURCE_DIR}/../reference/compare_original_inter2.py"
    --vendor "${P6D_VENDOR}"
    --cpp "$<TARGET_FILE:cteas_p6d_source_inter2_real4>"
    --out "${CMAKE_CURRENT_BINARY_DIR}/oracle_inter2_real4")
set_tests_properties(p6d_rebuild_independent_inter2_real4_real8_source
  PROPERTIES TIMEOUT 300)
