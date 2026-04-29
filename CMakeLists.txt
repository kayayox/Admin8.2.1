cmake_minimum_required(VERSION 3.14)
project(Admin8.2.1 VERSION 1.0.0 LANGUAGES CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable warnings (optional)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(MSVC)
    add_compile_options(/W4)
endif()

# Find SQLite3
find_package(SQLite3 REQUIRED)

# -----------------------------------------------------------------------------
# Collect all source files
# -----------------------------------------------------------------------------
# Main entry point
set(SOURCES main.cpp)

# Core modules
list(APPEND SOURCES
    core/ContextualCorrelator.cpp
    core/Learning.cpp
    core/word.cpp
)

# NLP / classification
list(APPEND SOURCES
    nlp/classifier.cpp
    nlp/morphology.cpp
)

# Tokenizer
list(APPEND SOURCES
    tokenizer/tokenizer.cpp
)

# Dialogue and pattern
list(APPEND SOURCES
    dialogue/PatternCorrelator.cpp
    dialogue/dialogue.cpp
    dialogue/pattern.cpp
)

# Database repositories
list(APPEND SOURCES
    db/db_manager.cpp
    db/dialogue_repository.cpp
    db/pattern_repository.cpp
    db/sentence_repository.cpp
    db/word_repository.cpp
)

# Utilities
list(APPEND SOURCES
    utils/PatternUtils.cpp
    utils/string_conv.cpp
    utils/tag_stats.cpp
)

# API implementation
list(APPEND SOURCES
    nlp_api.cpp
)

# -----------------------------------------------------------------------------
# Build executable
# -----------------------------------------------------------------------------
add_executable(${PROJECT_NAME} ${SOURCES})

# Set include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}          # for root-level headers
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/db
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogue
    ${CMAKE_CURRENT_SOURCE_DIR}/nlp
    ${CMAKE_CURRENT_SOURCE_DIR}/tokenizer
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

# Link SQLite3
target_link_libraries(${PROJECT_NAME} PRIVATE SQLite::SQLite3)

# (Optional) Copy required resource files if any
# file(COPY ... DESTINATION ${CMAKE_BINARY_DIR})

# Output build information
message(STATUS "Project: ${PROJECT_NAME} - Version ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "SQLite3 include: ${SQLite3_INCLUDE_DIRS}")
message(STATUS "SQLite3 lib: ${SQLite3_LIBRARIES}")

option(BUILD_TESTING "Construir tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
