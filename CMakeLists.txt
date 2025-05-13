cmake_minimum_required(VERSION 3.10)
project(ScreenCapture)

# 1. Set FFmpeg path (update to your actual installation)
set(FFMPEG_DIR "C:/ffmpeg")

# 2. Find ALL required FFmpeg components
find_path(AV_INCLUDE_DIR
        NAMES libavformat/avformat.h
        PATHS ${FFMPEG_DIR}/include
        REQUIRED
)

find_library(AVDEVICE_LIB avdevice PATHS ${FFMPEG_DIR}/lib REQUIRED)
find_library(AVFORMAT_LIB avformat PATHS ${FFMPEG_DIR}/lib REQUIRED)
find_library(AVCODEC_LIB avcodec PATHS ${FFMPEG_DIR}/lib REQUIRED)
find_library(SWSCALE_LIB swscale PATHS ${FFMPEG_DIR}/lib REQUIRED)
find_library(SWRESAMPLE_LIB swresample PATHS ${FFMPEG_DIR}/lib REQUIRED)  # Added this line
find_library(AVUTIL_LIB avutil PATHS ${FFMPEG_DIR}/lib REQUIRED)

# 3. Create executable
add_executable(ScreenCapture main.cpp)

# 4. Include directories
target_include_directories(ScreenCapture PRIVATE ${AV_INCLUDE_DIR})

# 5. Link libraries (CRITICAL ORDER!)
target_link_libraries(ScreenCapture PRIVATE
        ${AVDEVICE_LIB}  # Must come first!
        ${AVFORMAT_LIB}
        ${AVCODEC_LIB}
        ${SWSCALE_LIB}
        ${SWRESAMPLE_LIB}  # Added this line
        ${AVUTIL_LIB}
        ole32.lib
        oleaut32.lib
        user32.lib
        gdi32.lib
)

# 6. Copy required DLLs (verify your exact versions!)
set(FFMPEG_DLLS
        avdevice-62.dll
        avformat-62.dll
        avcodec-62.dll
        swscale-9.dll
        swresample-6.dll  # Added this line
        avutil-60.dll
)

foreach(DLL ${FFMPEG_DLLS})
    add_custom_command(TARGET ScreenCapture POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${FFMPEG_DIR}/bin/${DLL}"
            $<TARGET_FILE_DIR:ScreenCapture>
    )
endforeach()