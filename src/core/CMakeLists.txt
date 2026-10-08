add_library(core STATIC
        Types.h
        Board.h
        Board.cpp
        Rules.h
        Rules.cpp
        MoveHistory.h
        MoveHistory.cpp
        GameState.h
        GameState.cpp
        ITimeSource.h
        GameRecord.h
        GameRecord.cpp
        ReplayCursor.h
        ReplayCursor.cpp
)
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
