cmake_minimum_required(VERSION 3.20)
project(CTEAS_P6A_CALMET_CONTRACTS LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_library(cteas_p6a_contracts INTERFACE)
target_include_directories(cteas_p6a_contracts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
# G6 independent arbitrary-NX/NY GEO reader; frozen p6_read_geo/50 is unchanged.
add_executable(cteas_p6g_dynamic_geo_tests tests/p6g_dynamic_geo_tests.cpp)
target_link_libraries(cteas_p6g_dynamic_geo_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6g_dynamic_geo_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6g_dynamic_geo_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME p6g_dynamic_geo_utm_and_frozen_field_parity COMMAND cteas_p6g_dynamic_geo_tests)

# Source-compatible dynamic station deck, segregated from frozen SS1/US1.
add_executable(cteas_p6g_station_set_tests tests/p6g_station_set_tests.cpp)
target_link_libraries(cteas_p6g_station_set_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6g_station_set_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6g_station_set_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME p6g_multistation_readcf_real4_parity COMMAND cteas_p6g_station_set_tests)

# G7: native SURF.DAT v2.1 formatted multi-station and cross-year input.
# Frozen read_surface_2022 stays separate for independent 143h comparison.
add_executable(cteas_p6g_surface_dynamic_tests tests/p6g_surface_dynamic_tests.cpp)
target_link_libraries(cteas_p6g_surface_dynamic_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6g_surface_dynamic_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6g_surface_dynamic_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME p6g_surface_multi_site_multiyear_frozen_143h_parity COMMAND cteas_p6g_surface_dynamic_tests)

# G7b: native READ62/RDUPN upper-air file-per-station, multiple soundings.
add_executable(cteas_p6g_upper_dynamic_tests tests/p6g_upper_dynamic_tests.cpp)
target_link_libraries(cteas_p6g_upper_dynamic_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6g_upper_dynamic_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6g_upper_dynamic_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME p6g_upper_multi_site_multiyear_frozen_rdupn_parity COMMAND cteas_p6g_upper_dynamic_tests)

# G7c: SOURCE INPUT CONTRACT ONLY. No WIND1/COMP production physics.
add_executable(cteas_p6g_observation_input_tests tests/p6g_observation_input_tests.cpp)
target_link_libraries(cteas_p6g_observation_input_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6g_observation_input_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6g_observation_input_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME p6g_multistation_surface_upper_hourly_input_contract COMMAND cteas_p6g_observation_input_tests)

# G8: 10 km multi-SURF/multi-UP cross-year and full 8784h leap-year
# source-selected INTER2/SIMILT/VERTAV/PREPDI computational integration only.
# This is NOT full CALMET WIND1/DIAGNO physical equivalence.
add_executable(cteas_p6g_g8_10km_year_tests tests/p6g_g8_10km_year_stress_tests.cpp)
target_link_libraries(cteas_p6g_g8_10km_year_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6g_g8_10km_year_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6g_g8_10km_year_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME p6g_g8_multisite_10km_8784h_selected_source_bridge COMMAND cteas_p6g_g8_10km_year_tests)
set_tests_properties(p6g_g8_multisite_10km_8784h_selected_source_bridge PROPERTIES TIMEOUT 900)




add_executable(cteas_p6a_contract_tests tests/p6_contract_tests.cpp)
target_link_libraries(cteas_p6a_contract_tests PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_terset src/terrain_envelope_main.cpp)
target_link_libraries(cteas_p6_terset PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_terrain_envelope_tests tests/p6_terrain_envelope_tests.cpp)
target_link_libraries(cteas_p6_terrain_envelope_tests PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_fradj src/froude_adjust_main.cpp)
target_link_libraries(cteas_p6_fradj PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_fradj_tests tests/p6_froude_adjust_tests.cpp)
target_link_libraries(cteas_p6_fradj_tests PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_stations src/station_geometry_main.cpp)
target_link_libraries(cteas_p6_stations PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_station_tests tests/p6_station_geometry_tests.cpp)
target_link_libraries(cteas_p6_station_tests PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_read_geo src/read_geo_main.cpp)
target_link_libraries(cteas_p6_read_geo PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_geo_tests tests/p6_read_geo_tests.cpp)
target_link_libraries(cteas_p6_geo_tests PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_read_surface src/read_surface_main.cpp)
target_link_libraries(cteas_p6_read_surface PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_surface_tests tests/p6_read_surface_tests.cpp)
target_link_libraries(cteas_p6_surface_tests PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_read_upper src/read_upper_main.cpp)
target_link_libraries(cteas_p6_read_upper PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_upper_tests tests/p6_read_upper_tests.cpp)
target_link_libraries(cteas_p6_upper_tests PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_vertav src/vertical_wind_average_main.cpp)
target_link_libraries(cteas_p6_vertav PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_vertav_tests tests/p6_vertical_wind_average_tests.cpp)
target_link_libraries(cteas_p6_vertav_tests PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_upper_hourly src/upper_hourly_wind_main.cpp)
target_link_libraries(cteas_p6_upper_hourly PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_hourly_tests tests/p6_upper_hourly_wind_tests.cpp)
target_link_libraries(cteas_p6_hourly_tests PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_facet src/temperature_faces_main.cpp)
target_link_libraries(cteas_p6_facet PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_facet_tests tests/p6_temperature_faces_tests.cpp)
target_link_libraries(cteas_p6_facet_tests PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_comp_joint src/comp_joint_main.cpp)
target_link_libraries(cteas_p6_comp_joint PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_comp_buffered_tests tests/p6_comp_buffered_tests.cpp)
target_link_libraries(cteas_p6_comp_buffered_tests PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_rdsn_buffered_tests tests/p6_rdsn_buffered_tests.cpp)
target_link_libraries(cteas_p6_rdsn_buffered_tests PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_cgamma src/cgamma_main.cpp)
target_link_libraries(cteas_p6_cgamma PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_cgamma_tests tests/p6_cgamma_tests.cpp)
target_link_libraries(cteas_p6_cgamma_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6_cgamma PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_cgamma_tests PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_rdsn_buffered_tests PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_comp_joint PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_comp_buffered_tests PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_facet PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_facet_tests PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_upper_hourly PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_hourly_tests PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_vertav PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_vertav_tests PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_read_upper PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_upper_tests PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_read_surface PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_surface_tests PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_read_geo PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_geo_tests PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_stations PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_station_tests PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_fradj PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_fradj_tests PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_terset PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_terrain_envelope_tests PRIVATE /W4 /WX)
  target_compile_options(cteas_p6a_contract_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6_cgamma PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_cgamma_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_rdsn_buffered_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_comp_joint PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_comp_buffered_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6a_contract_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_terset PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_terrain_envelope_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_fradj PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_fradj_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_stations PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_station_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_read_geo PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_geo_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_read_surface PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_surface_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_read_upper PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_upper_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_vertav PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_vertav_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_upper_hourly PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_hourly_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_facet PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_facet_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
enable_testing()
add_test(NAME p6a_calmet_scoped_contracts COMMAND cteas_p6a_contract_tests)
add_test(NAME p6c_terrain_envelope_software COMMAND cteas_p6_terrain_envelope_tests)
add_test(NAME p6d_froude_adjust_software COMMAND cteas_p6_fradj_tests)
add_test(NAME p6c_station_geometry_software COMMAND cteas_p6_station_tests)
add_test(NAME p6c_geo_selected_scoped_software COMMAND cteas_p6_geo_tests)
add_test(NAME p6c_surface_selected_scoped_software COMMAND cteas_p6_surface_tests)
add_test(NAME p6c_upper_rdupn_selected_scoped_software COMMAND cteas_p6_upper_tests)
add_test(NAME p6c_original_vertav_software COMMAND cteas_p6_vertav_tests)
add_test(NAME p6c_hourly_prepdi_software COMMAND cteas_p6_hourly_tests)
add_test(NAME p6c_original_facet_intp_software COMMAND cteas_p6_facet_tests)
add_test(NAME p6c_comp_buffered_candidate_software COMMAND cteas_p6_comp_buffered_tests)
add_test(NAME p6c_rdsn_buffered_state_candidate COMMAND cteas_p6_rdsn_buffered_tests)
add_test(NAME p6c_original_cgamma_state_candidate COMMAND cteas_p6_cgamma_tests)

# P6D independent SLOPE kernel -- not a diagnostic-wind integration claim.
add_executable(cteas_p6_slope src/slope_flow_main.cpp)
target_link_libraries(cteas_p6_slope PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_slope_tests tests/p6_slope_flow_tests.cpp)
target_link_libraries(cteas_p6_slope_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6_slope PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_slope_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6_slope PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_slope_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME p6d_original_slope_software COMMAND cteas_p6_slope_tests)

# P6D DIVCEL source-scoped divergence kernel. MINIM not yet integrated.
add_executable(cteas_p6_divcel src/divcel_main.cpp)
target_link_libraries(cteas_p6_divcel PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_divcel_tests tests/p6_divcel_tests.cpp)
target_link_libraries(cteas_p6_divcel_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6_divcel PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_divcel_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6_divcel PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_divcel_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME p6d_original_divcel_software COMMAND cteas_p6_divcel_tests)

# P6D source-scoped MINIM + WINDBC, using independently verified DIVCEL.
add_executable(cteas_p6_minim src/minim_main.cpp)
target_link_libraries(cteas_p6_minim PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_minim_tests tests/p6_minim_tests.cpp)
target_link_libraries(cteas_p6_minim_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6_minim PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_minim_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6_minim PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_minim_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME p6d_original_minim_software COMMAND cteas_p6_minim_tests)

# P6D selected non-lake-breeze SMOOTH, original Fortran parity gate.
add_executable(cteas_p6_smooth src/smooth_main.cpp)
target_link_libraries(cteas_p6_smooth PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_smooth_tests tests/p6_smooth_tests.cpp)
target_link_libraries(cteas_p6_smooth_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6_smooth PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_smooth_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6_smooth PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_smooth_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME p6d_original_smooth_software COMMAND cteas_p6_smooth_tests)

# P6D selected DIAGNO L6119-L6148 SMOOTH+DIVCEL+W+WINDBC segment.
add_executable(cteas_p6_final_segment src/diagno_segment_main.cpp)
target_link_libraries(cteas_p6_final_segment PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_final_segment_tests tests/p6_diagno_segment_tests.cpp)
target_link_libraries(cteas_p6_final_segment_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6_final_segment PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_final_segment_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6_final_segment PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_final_segment_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME p6d_original_diagno_final_segment_software COMMAND cteas_p6_final_segment_tests)

# P6D selected DIAGNO INTER2, real observation/first-guess route, not full WIND1.
add_executable(cteas_p6_inter2 src/inter2_selected_main.cpp)
target_link_libraries(cteas_p6_inter2 PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_inter2_tests tests/p6_inter2_selected_tests.cpp)
target_link_libraries(cteas_p6_inter2_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6_inter2 PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_inter2_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6_inter2 PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_inter2_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME p6d_original_inter2_selected_software COMMAND cteas_p6_inter2_tests)

# P6D selected WIND1 IEXTRP=-4 calls neutral SIMILT (not full WIND1).
add_executable(cteas_p6_similt_profile src/similt_profile_main.cpp)
target_link_libraries(cteas_p6_similt_profile PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_similt_profile_tests tests/p6_similt_profile_tests.cpp)
target_link_libraries(cteas_p6_similt_profile_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6_similt_profile PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_similt_profile_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6_similt_profile PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_similt_profile_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME p6d_original_similt_profile_software COMMAND cteas_p6_similt_profile_tests)

# P6D8A scoped WIND1 surface/upper observation wiring; not full WIND1 parity.
add_executable(cteas_p6_wind1_hour_tests tests/p6_wind1_selected_station_hour_tests.cpp)
target_link_libraries(cteas_p6_wind1_hour_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6_wind1_hour_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6_wind1_hour_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME p6d8_wind1_selected_hour_observation_contract COMMAND cteas_p6_wind1_hour_tests)

# P6D8B selected original WIND1 first-guess grid (not full DIAGNO).
add_executable(cteas_p6_wind1_firstguess src/wind1_firstguess_main.cpp)
target_link_libraries(cteas_p6_wind1_firstguess PRIVATE cteas_p6a_contracts)
add_executable(cteas_p6_wind1_firstguess_tests tests/p6_wind1_selected_firstguess_tests.cpp)
target_link_libraries(cteas_p6_wind1_firstguess_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6_wind1_firstguess PRIVATE /W4 /WX)
  target_compile_options(cteas_p6_wind1_firstguess_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6_wind1_firstguess PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(cteas_p6_wind1_firstguess_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME p6d8b_original_wind1_firstguess_selected_software COMMAND cteas_p6_wind1_firstguess_tests)

# P6D9A source-scoped WIND1 -> INTER2 -> SMOOTH/DIVCEL/W bridge.
# Heat/terrain/slope effects MUST be supplied externally or explicitly bypassed
# in synthetic source-only audit mode; this does not qualify full DIAGNO.
add_executable(cteas_p6_d9a_bridge_tests tests/p6_diagno_d9a_bridge_tests.cpp)
target_link_libraries(cteas_p6_d9a_bridge_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6_d9a_bridge_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6_d9a_bridge_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME p6d9a_wind1_inter2_diagno_selected_bridge_contract COMMAND cteas_p6_d9a_bridge_tests)

# P6D9A selected DIAGNO IOBR=1 tail: original L6153-L6159 O'Brien W
# adjustment plus L6180 MINIM. Upstream terrain/heat/slope still excluded.
add_executable(cteas_p6_obrien_minim_tests tests/p6_diagno_obrien_minim_tests.cpp)
target_link_libraries(cteas_p6_obrien_minim_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6_obrien_minim_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6_obrien_minim_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME p6d9a_obrien_minim_iobr1_tail_contract COMMAND cteas_p6_obrien_minim_tests)

# P6D9A selected origin IFRADJ=1 upstream route: original FRADJ L5643-L5644
# on the step-one winds, before the L5748 boundary snapshot and INTER2.
add_executable(cteas_p6_fradj_route_tests tests/p6_diagno_fradj_route_tests.cpp)
target_link_libraries(cteas_p6_fradj_route_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6_fradj_route_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6_fradj_route_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME p6d9a_fradj_upstream_route_contract COMMAND cteas_p6_fradj_route_tests)

# P6D9A selected original ISLOPE=1 upstream route: SLOPE L5722-L5723 plus the
# L5727-L5732 additive loop after FRADJ, before L5748/L6055/L6059.
add_executable(cteas_p6_slope_route_tests tests/p6_diagno_slope_route_tests.cpp)
target_link_libraries(cteas_p6_slope_route_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6_slope_route_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6_slope_route_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME p6d9a_slope_upstream_route_contract COMMAND cteas_p6_slope_route_tests)

# P6D9A selected original IKINE=1 pre-blend kinematic route: W zeroing
# L5615-L5616, TOPOF2 L5621-L5622 and MINIM L5628-L5629.
add_executable(cteas_p6_topof2_route_tests tests/p6_diagno_topof2_route_tests.cpp)
target_link_libraries(cteas_p6_topof2_route_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6_topof2_route_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6_topof2_route_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME p6d9a_topof2_preblend_route_contract COMMAND cteas_p6_topof2_route_tests)

# P6D9A in-chain original TERSET envelope (DIAGI L5228, I3DCTW=IWFCOD=1 in the
# real deck) replacing the caller-supplied HMAX/HMIN/DISTH* planes.
add_executable(cteas_p6_terset_envelope_tests tests/p6_diagno_terset_envelope_tests.cpp)
target_link_libraries(cteas_p6_terset_envelope_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6_terset_envelope_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6_terset_envelope_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME p6d9a_inchain_terset_envelope_contract COMMAND cteas_p6_terset_envelope_tests)

# Original CALMET met.inp station positions: UTM kilometres -> REAL*4
# relative metres (not the auxiliary CSV geographic projection).
add_executable(cteas_p6_station_deck_tests tests/p6_station_deck_input_tests.cpp)
target_link_libraries(cteas_p6_station_deck_tests PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6_station_deck_tests PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6_station_deck_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
# Isolated P6 kernel workflows sometimes fetch just C++ sources, not the
# multi-gigabyte frozen Hefei case. They must still test the source in isolation
# and must NOT claim real-case verification. The dedicated Mode A job requires
# full case inputs and fails configuration when any required artifact is absent.
option(CTEAS_P6_MODE_A_REQUIRE_CASE
       "Require complete frozen Hefei input tree for P6 Mode A numerical gates" OFF)
set(P6_MODEA_CASE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../project/cases/hefei5km_2022_reference_case_20261011")
set(P6_MODEA_REQUIRED_FILES
    "case.yaml"
    "grid/grid.cmn"
    "grid/center_location"
    "geo/terrel/terrel.inp"
    "geo/ctgproc/ctgproc.inp"
    "geo/makegeo/makegeo.inp"
    "geo/makegeo/geo.dat"
    "run/met/met.inp"
    "met/surface/final/stations.csv"
    "met/surface/final/surf.dat"
    "met/upperair/final/stations.csv"
    "met/upperair/final/up_58424.dat")
set(P6_MODEA_CASE_READY TRUE)
foreach(item IN LISTS P6_MODEA_REQUIRED_FILES)
  if(NOT EXISTS "${P6_MODEA_CASE}/${item}")
    set(P6_MODEA_CASE_READY FALSE)
    if(CTEAS_P6_MODE_A_REQUIRE_CASE)
      message(FATAL_ERROR "P6 Mode A required frozen-case input missing: ${P6_MODEA_CASE}/${item}")
    endif()
  endif()
endforeach()
add_test(NAME p6d_original_station_deck_synthetic_source_contract
         COMMAND cteas_p6_station_deck_tests)
if(P6_MODEA_CASE_READY)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  add_test(NAME p6d_original_metinp_station_deck_input
           COMMAND cteas_p6_station_deck_tests "${P6_MODEA_CASE}/run/met/met.inp")
  add_test(NAME p6d_p3_p4_p5_metinp_station_contract
           COMMAND ${Python3_EXECUTABLE}
             "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_case_station_contract.py"
             "${P6_MODEA_CASE}")
  add_test(NAME p6_modea_original_grid_generator_to_metinp
           COMMAND ${Python3_EXECUTABLE}
             "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_case_grid_source_contract.py"
             "${P6_MODEA_CASE}")
  find_program(P6_STATION_GFORTRAN NAMES gfortran-11 gfortran-12 gfortran)
  if(P6_STATION_GFORTRAN)
    add_test(NAME p6d_original_readcf_real4_fortran_source
             COMMAND ${Python3_EXECUTABLE}
               "${CMAKE_CURRENT_SOURCE_DIR}/reference/compare_original_deck_station.py"
               --vendor "${CMAKE_CURRENT_SOURCE_DIR}/../../../engine/vendor/src/CALMET_v6.5.0_L150223/calmet.for"
               --deck "${P6_MODEA_CASE}/run/met/met.inp"
               --compiler ${P6_STATION_GFORTRAN}
               --out "${CMAKE_CURRENT_BINARY_DIR}/original_readcf_station")
  elseif(CTEAS_P6_MODE_A_REQUIRE_CASE)
    message(FATAL_ERROR "P6 Mode A original Fortran source oracle requires gfortran")
  else()
    message(STATUS "P6 original Fortran source oracle unavailable; not certified")
  endif()
else()
  message(STATUS "P6 Mode A full-case tests NOT REGISTERED; source-only CTests are still valid")
endif()

# P6D real-case wind-field producer (partial product: WIND1 step-one UG/VG
# plus the in-chain TERSET envelope, with explicit provenance metadata).
add_executable(cteas_p6_realcase_wind_field src/realcase_wind_field_main.cpp)
target_link_libraries(cteas_p6_realcase_wind_field PRIVATE cteas_p6a_contracts)
# Read-only reuse of the P6E submodule headers (SURFVAR/SOLAR/HEATFX); no P6E
# source is modified and the P6E module keeps its own build.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/p6e/include)
  target_include_directories(cteas_p6_realcase_wind_field PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/p6e/include)
else()
  message(FATAL_ERROR "P6E submodule headers required by the real-case producer are missing")
endif()
if(MSVC)
  target_compile_options(cteas_p6_realcase_wind_field PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6_realcase_wind_field PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Fail-closed contract run of the real-case producer on the frozen 5 km case
# tree (read-only inputs; outputs go to the build tree). The producer throws on
# any missing or invalid case input, so exit status plus the emitted hour count
# is the whole assertion.
if(P6_MODEA_CASE_READY)
  add_test(NAME p6d_realcase_wind_producer_case_contract
           COMMAND cteas_p6_realcase_wind_field
                   --case "${P6_MODEA_CASE}"
                   --out ${CMAKE_CURRENT_BINARY_DIR}/realcase_wind_ctest)
  set_tests_properties(p6d_realcase_wind_producer_case_contract PROPERTIES
                       PASS_REGULAR_EXPRESSION "hours=143")
endif()

# P6D9A composed trace for chained unmodified original Fortran comparisons.
add_executable(cteas_p6_d9a_source_trace src/diagno_d9a_trace_main.cpp)
target_link_libraries(cteas_p6_d9a_source_trace PRIVATE cteas_p6a_contracts)
if(MSVC)
  target_compile_options(cteas_p6_d9a_source_trace PRIVATE /W4 /WX)
else()
  target_compile_options(cteas_p6_d9a_source_trace PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
