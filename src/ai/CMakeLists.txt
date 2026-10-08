add_library(ai STATIC
        Evaluator.h
        Evaluator.cpp
        Candidates.h
        Candidates.cpp
)
target_include_directories(ai PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ai PUBLIC core)
