# CMake generated Testfile for 
# Source directory: C:/work/KernelSocket-API/tests/testDispatcher
# Build directory: C:/work/KernelSocket-API/tests/testDispatcher/build-windows
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(ksockapi_test_dispatcher "C:/work/KernelSocket-API/tests/testDispatcher/build-windows/ksockapi_test_dispatcher.exe")
set_tests_properties(ksockapi_test_dispatcher PROPERTIES  _BACKTRACE_TRIPLES "C:/work/KernelSocket-API/tests/testDispatcher/CMakeLists.txt;50;add_test;C:/work/KernelSocket-API/tests/testDispatcher/CMakeLists.txt;0;")
subdirs("googletest_build")
