cmake_minimum_required(VERSION 3.16)
project(CSCAN LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus")

# Force vcpkg toolchain
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

set(CMAKE_PREFIX_PATH "C:/Qt/6.9.1/msvc2022_64")

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

# Manual Tesseract paths with correct library names
set(TESSERACT_ROOT "C:/vcpkg/installed/x64-windows")
find_path(TESSERACT_INCLUDE_DIR tesseract/baseapi.h PATHS ${TESSERACT_ROOT}/include)
find_library(TESSERACT_LIB NAMES tesseract55 tesseract PATHS ${TESSERACT_ROOT}/lib)
find_library(LEPTONICA_LIB NAMES leptonica-1.85.0 leptonica PATHS ${TESSERACT_ROOT}/lib)

if(TESSERACT_INCLUDE_DIR AND TESSERACT_LIB AND LEPTONICA_LIB)
    set(TESSERACT_FOUND TRUE)
    message(STATUS "✅ Tesseract found at: ${TESSERACT_ROOT}")
    message(STATUS "✅ Tesseract lib: ${TESSERACT_LIB}")
    message(STATUS "✅ Leptonica lib: ${LEPTONICA_LIB}")
else()
    set(TESSERACT_FOUND FALSE)
    message(STATUS "❌ Tesseract not found")
    message(STATUS "Include: ${TESSERACT_INCLUDE_DIR}")
    message(STATUS "Tesseract lib: ${TESSERACT_LIB}")
    message(STATUS "Leptonica lib: ${LEPTONICA_LIB}")
endif()

set(CMAKE_AUTOMOC ON)

add_executable(CSCAN 
    main.cpp 
    MainWindow.cpp 
    MainWindow.h
    ScreenCapture.cpp
    ScreenCapture.h
    OCRProcessor.cpp
    OCRProcessor.h
    EconomyEngine.cpp
    EconomyEngine.h
)

target_link_libraries(CSCAN
    Qt6::Core
    Qt6::Widgets  
    Qt6::Gui
)

# Add Tesseract if found
if(TESSERACT_FOUND)
    target_include_directories(CSCAN PRIVATE ${TESSERACT_INCLUDE_DIR})
    target_link_libraries(CSCAN ${TESSERACT_LIB} ${LEPTONICA_LIB})
    target_compile_definitions(CSCAN PRIVATE TESSERACT_AVAILABLE)
    message(STATUS "🎯 Real OCR enabled!")
else()
    message(STATUS "📝 Using mock OCR")
endif()

if(WIN32)
    target_link_libraries(CSCAN user32 gdi32 dwmapi)
endif()