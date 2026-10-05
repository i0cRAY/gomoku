add_library(format STATIC
        SaveFormat.h
        SaveFormat.cpp
)
target_include_directories(format PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(format PUBLIC core Qt6::Core)
