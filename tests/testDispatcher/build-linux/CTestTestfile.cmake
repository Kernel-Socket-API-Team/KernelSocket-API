# CMake generated Testfile for 
# Source directory: /mnt/c/work/KernelSocket-API/tests/testDispatcher
# Build directory: /mnt/c/work/KernelSocket-API/tests/testDispatcher/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(ksockapi_test_dispatcher "/mnt/c/work/KernelSocket-API/tests/testDispatcher/build/ksockapi_test_dispatcher")
set_tests_properties(ksockapi_test_dispatcher PROPERTIES  _BACKTRACE_TRIPLES "/mnt/c/work/KernelSocket-API/tests/testDispatcher/CMakeLists.txt;39;add_test;/mnt/c/work/KernelSocket-API/tests/testDispatcher/CMakeLists.txt;0;")
subdirs("googletest_build")
