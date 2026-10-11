cmake_minimum_required(VERSION 3.20)
project(CTEAS_P6F_CALMET_DAT21_WRITER LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# P6F: CALMET.DAT 2.1 writer.
#
#   p6f_calmet_dat21            -- the writer library (explicit arrays -> bytes)
#   cteas_p6f_calmet_dat21_writer -- CLI: roundtrip | check | emit-fixture
#
# The module is standalone: it includes no P6E/P6D/P7 header. The E22 output
# contract of the P6E module is consumed only by the cross-read test below,
# through an explicit read-only include path, as an independent decoder.
# ---------------------------------------------------------------------------
add_library(p6f_calmet_dat21 STATIC
  src/calmet_dat21_writer.cpp
  src/sha256.cpp)
target_include_directories(p6f_calmet_dat21 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(cteas_p6f_calmet_dat21_writer src/calmet_dat21_writer_main.cpp)
target_link_libraries(cteas_p6f_calmet_dat21_writer PRIVATE p6f_calmet_dat21)

add_executable(p6f_calmet_dat21_writer_tests tests/calmet_dat21_writer_tests.cpp)
target_link_libraries(p6f_calmet_dat21_writer_tests PRIVATE p6f_calmet_dat21)

set(P6F_E22_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../p6e/include")
if(NOT EXISTS "${P6F_E22_INCLUDE}/cteas/p6e/calmet_dat21.hpp")
  message(FATAL_ERROR
    "P6F cross-read needs the E22 output contract header "
    "${P6F_E22_INCLUDE}/cteas/p6e/calmet_dat21.hpp (read-only dependency).")
endif()
add_executable(p6f_calmet_dat21_e22_cross_read_tests
  tests/calmet_dat21_e22_cross_read_tests.cpp)
target_link_libraries(p6f_calmet_dat21_e22_cross_read_tests PRIVATE p6f_calmet_dat21)
target_include_directories(p6f_calmet_dat21_e22_cross_read_tests PRIVATE ${P6F_E22_INCLUDE})

foreach(target p6f_calmet_dat21 cteas_p6f_calmet_dat21_writer
        p6f_calmet_dat21_writer_tests p6f_calmet_dat21_e22_cross_read_tests)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
endforeach()

enable_testing()
add_test(NAME p6f_calmet_dat21_writer_unit COMMAND p6f_calmet_dat21_writer_tests)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Synthetic CLI end-to-end: emit-fixture -> check -> roundtrip, with an
# independent Python walk of the produced bytes and the fail-closed cases.
add_test(NAME p6f_calmet_dat21_synthetic_cli_roundtrip
  COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_synthetic_cli_roundtrip.py"
    --tool "$<TARGET_FILE:cteas_p6f_calmet_dat21_writer>"
    --report "${CMAKE_CURRENT_BINARY_DIR}/p6f-synthetic-report.json")

# The frozen real 2022 reference case and the independent P7 decoder are the
# writer's ground truth: the gate is FAIL-closed when either is missing rather
# than silently skipping the byte-identity proof.
set(P6F_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")
set(P6F_REAL_MET_DAT
  "${P6F_REPO_ROOT}/project/cases/hefei5km_2022_reference_case_20261011/run/met/met.dat")
set(P6F_REAL_MET_SHA256
  "b1823cda92280888d70b00961ab39486036a3086e18637fff21efb4f4b237d30")
set(P6F_P7_DECODER
  "${CMAKE_CURRENT_SOURCE_DIR}/../../p7_calpuff/reference/original_decode_calmet_hourly.py")
if(NOT EXISTS "${P6F_REAL_MET_DAT}")
  message(FATAL_ERROR
    "P6F byte-identity acceptance needs the frozen reference ${P6F_REAL_MET_DAT}.")
endif()
if(NOT EXISTS "${P6F_P7_DECODER}")
  message(FATAL_ERROR
    "P6F cross-read acceptance needs the P7 decoder ${P6F_P7_DECODER}.")
endif()

# read -> rewrite -> byte-for-byte identical (SHA-256 pinned) + independent
# inventory + P7 decoder cross-read + fail-closed negatives.
add_test(NAME p6f_calmet_dat21_real_reference_round_trip
  COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_real_reference_roundtrip.py"
    --tool "$<TARGET_FILE:cteas_p6f_calmet_dat21_writer>"
    --p7-decoder "${P6F_P7_DECODER}"
    --real-reference "${P6F_REAL_MET_DAT}"
    --real-reference-sha256 "${P6F_REAL_MET_SHA256}"
    --report "${CMAKE_CURRENT_BINARY_DIR}/p6f-real-reference-report.json")

# The E22 contract reader must parse every record of the re-serialised frozen
# case without loss (6,435 field records compared cell by cell).
add_test(NAME p6f_calmet_dat21_e22_contract_cross_read
  COMMAND p6f_calmet_dat21_e22_cross_read_tests "${P6F_REAL_MET_DAT}")
