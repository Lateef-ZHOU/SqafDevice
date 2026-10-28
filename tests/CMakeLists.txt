file(GLOB_RECURSE headers "${CMAKE_CURRENT_SOURCE_DIR}/../include/*.h")
add_executable(SqafDevice_smoke smoke.cpp)
target_link_libraries(SqafDevice_smoke PRIVATE Lidar::SqafDevice LidarSqafDeviceplugin
    LidarTaskUIplugin LidarUICoreplugin LidarChartCoreplugin)

# 直接编译生产 Handle 与固定依赖自带的串口替身，不打开实物串口。
add_executable(SqafDevice_serial pelco_protocol.cpp
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/Device/Handle/PelcoDHandle.h"
    "${lidar_handle_core_SOURCE_DIR}/tests/serial/FakeSerialPort.cpp")
target_include_directories(SqafDevice_serial PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    "${lidar_handle_core_SOURCE_DIR}/tests/serial")
target_link_libraries(SqafDevice_serial PRIVATE Lidar::Serial Boost::circular_buffer)
target_compile_definitions(SqafDevice_serial PRIVATE QT_NO_KEYWORDS)
if(WIN32)
    target_compile_definitions(SqafDevice_serial PRIVATE LIBSERIALPORT_MSBUILD)
endif()
set(units)
foreach(header IN LISTS headers)
    file(RELATIVE_PATH relative "${CMAKE_CURRENT_SOURCE_DIR}/../include" "${header}")
    string(MAKE_C_IDENTIFIER "${relative}" unit)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${unit}.cpp" "#include <${relative}>\n")
    list(APPEND units "${CMAKE_CURRENT_BINARY_DIR}/${unit}.cpp")
endforeach()
add_library(SqafDevice_headers OBJECT ${units})
target_link_libraries(SqafDevice_headers PRIVATE Lidar::SqafDevice)
foreach(target SqafDevice_headers SqafDevice_smoke SqafDevice_serial)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endforeach()
add_test(NAME SqafDevice_page COMMAND SqafDevice_smoke)
add_test(NAME SqafDevice_serial COMMAND SqafDevice_serial)
set_tests_properties(SqafDevice_page SqafDevice_serial PROPERTIES TIMEOUT 30
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QSG_RHI_BACKEND=null"
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
if(WIN32)
    set_property(TEST SqafDevice_page SqafDevice_serial APPEND PROPERTY ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:libserialport::libserialport>")
endif()
