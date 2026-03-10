cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Дополнительные зависимости для компиляции под Windows
if(WIN32 AND MINGW)
    # Сборка: cmake .. -G "MinGW Makefiles"
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++17")
endif()

project(ksockapi_test_dispatcher LANGUAGES C CXX)

# Подключаем googletest из submodule
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../libraries/googletest googletest_build)

# Подключаем директории с заголовками
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libraries/googletest/googletest/include)

# Все .c файлы проекта
set(LIB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/dispatcher/dispatcher.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/adapters/windows/windows_adapter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/adapters/linux/linux_adapter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_stubs_linux.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_stubs_windows.cpp
)

# Исполняемый файл для тестов
add_executable(ksockapi_test_dispatcher
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_dispatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/GlobalContext/GlobalContext.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/adapters/windows/windows_adapter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/adapters/linux/linux_adapter.c
    ${LIB_SOURCES}
)

# Линковка с Google Test
target_link_libraries(ksockapi_test_dispatcher
    gtest
    gtest_main
    pthread
)

# Включаем тестирование
enable_testing()
add_test(NAME ksockapi_test_dispatcher COMMAND ksockapi_test_dispatcher)