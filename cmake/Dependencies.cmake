include(FetchContent)
if(NOT TARGET Boost::circular_buffer)
    find_package(Boost 1.88 QUIET CONFIG COMPONENTS circular_buffer)
    if(NOT TARGET Boost::circular_buffer)
        set(BOOST_INCLUDE_LIBRARIES ${BOOST_INCLUDE_LIBRARIES} circular_buffer)
        FetchContent_Declare(boost
            URL https://github.com/boostorg/boost/releases/download/boost-1.88.0/boost-1.88.0-cmake.tar.xz
            URL_HASH SHA256=f48b48390380cfb94a629872346e3a81370dc498896f16019ade727ab72eb1ec
            EXCLUDE_FROM_ALL)
        FetchContent_MakeAvailable(boost)
    endif()
endif()
set(HANDLECORE_WITH_SERIAL ON)
if(NOT TARGET Lidar::HandleCore)
    FetchContent_Declare(lidar_handle_core
        GIT_REPOSITORY https://github.com/Lateef-ZHOU/HandleCore.git
        GIT_TAG 4a2bb97d8ed1b719336039fc9bd2db3d1f73f8e9)
    FetchContent_MakeAvailable(lidar_handle_core)
endif()
if(NOT TARGET Lidar::Serial)
    message(FATAL_ERROR "Enable HANDLECORE_WITH_SERIAL before the first HandleCore configuration")
endif()
set(TASKCORE_WITH_QML ON)
if(NOT TARGET Lidar::TaskUI)
    FetchContent_Declare(lidar_taskcore
        GIT_REPOSITORY https://github.com/Lateef-ZHOU/TaskCore.git
        GIT_TAG 49c33b251af6b487f63d81f63bb61ce01f11116c)
    FetchContent_MakeAvailable(lidar_taskcore)
endif()
