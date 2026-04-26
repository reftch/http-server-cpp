add_executable (tests tests.cpp)
# target_link_libraries (tests http-server)
target_link_libraries(tests server)

# create test target name
# set(TARGET TestLib)
# # create test executable
# add_executable(${TARGET} test_server.cpp)
# # link our testd lib MyLib to the test executable declared above
# # target_link_libraries(${TARGET} PRIVATE gtest_main MyLib)
# # define a test with its name and command to run it - in our simple case,
# # test name is the same as command to run the test (the executable), but in
# # command you can pass more information, e.g. the command line arguments
# add_test(NAME ${TARGET} COMMAND ${TARGET})