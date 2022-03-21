cmake_minimum_required(VERSION 3.13)
project( tauschema_test C )
set( CMAKE_C_STANDARD 11 )

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
message("!!! Image output directory: " ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

set( ENV{PATH} "${GNU_RM_INSTALL_DIR}/bin:$ENV{PATH}" )

# Test target 

add_executable( bin_c_test )
target_compile_options( bin_c_test PRIVATE -Wall -ftest-coverage -fprofile-arcs -O0 )
target_compile_definitions( bin_c_test PRIVATE tsch_size_t=uint32_t )
target_link_options( bin_c_test PRIVATE -ftest-coverage -fprofile-arcs )
add_custom_command( TARGET bin_c_test 
	COMMAND lcov -z -d ${CMAKE_BINARY_DIR}
	COMMAND mkdir -p ${CMAKE_SOURCE_DIR}/reports
	COMMAND bin_c_test | tee ${CMAKE_SOURCE_DIR}/reports/stdout_bin_c_test.txt
	COMMAND geninfo ${CMAKE_BINARY_DIR} -o coverage_bin_c_test.info
	COMMAND genhtml coverage_bin_c_test.info -o ${CMAKE_SOURCE_DIR}/reports/bin_c_test/
	)

target_include_directories( bin_c_test PRIVATE . ../src )
target_sources( bin_c_test PRIVATE 
	../src/tauschema_codec.c 
	../src/tauschema_check.c 
	test_buf.c test_flater.c testmain.c 
	tauschema_device_info_schema.c
	)
