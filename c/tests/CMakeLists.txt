include(CTest)
include(FetchContent)

FetchContent_Declare(
	catch
	GIT_REPOSITORY https://github.com/catchorg/Catch2.git
	GIT_TAG v3.8.0
)
FetchContent_MakeAvailable(catch)

add_executable(tests
	# Add test files here
    measure/timeseries.cpp
)

# file(COPY testfiles ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(tests PRIVATE Catch2::Catch2WithMain tirex::tracker)
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../src)
target_compile_features(tests PRIVATE cxx_std_20)

add_test(NAME tirextrackertest COMMAND tests
	#WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/testfiles
)