cmake_minimum_required(VERSION 3.31)
project(BeautifulString C)

set(CMAKE_C_STANDARD 17)

add_definitions(-DNDEBUG)

set(Python_ROOT_DIR "C:/Users/Juha/AppData/Local/Programs/Python/Python313/")  # Customize accordingly
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

add_library(BeautifulString MODULE library.c strscan.c strfetch.c strmatch.c strsearch.c
        strmatch_internal.h
        strmatch_internal.c)

target_include_directories(BeautifulString PRIVATE ${Python3_INCLUDE_DIRS})
target_link_libraries(BeautifulString PRIVATE ${Python3_LIBRARIES})

set_target_properties(BeautifulString PROPERTIES PREFIX "" SUFFIX ".pyd" OUTPUT_NAME "BeautifulString")  # For Python on Windows

if (MSVC)
    target_compile_definitions(BeautifulString PRIVATE
            $<$<CONFIG:Debug>:_CRT_SECURE_NO_WARNINGS>
            $<$<CONFIG:Debug>:_HAS_EXCEPTIONS=0>
            $<$<CONFIG:Debug>:U_DEBUG>   # Undefine _DEBUG!
            $<$<CONFIG:Debug>:NDEBUG>    # Pretend it's release
    )
endif()