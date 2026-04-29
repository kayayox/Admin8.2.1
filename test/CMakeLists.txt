cmake_minimum_required(VERSION 3.14)

# -----------------------------------------------------------------------
# Obtener Catch2
# -----------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(Catch2)

# Necesario para catch_discover_tests
include(CTest)
enable_testing()

# -----------------------------------------------------------------------
# Fuentes del proyecto (sin main.cpp)
# -----------------------------------------------------------------------
set(PROJECT_BASE ${CMAKE_SOURCE_DIR})
set(PROJECT_SOURCES
    ${PROJECT_BASE}/core/word.cpp
    ${PROJECT_BASE}/core/ContextualCorrelator.cpp
    ${PROJECT_BASE}/core/Learning.cpp
    ${PROJECT_BASE}/nlp/classifier.cpp
    ${PROJECT_BASE}/nlp/morphology.cpp
    ${PROJECT_BASE}/tokenizer/tokenizer.cpp
    ${PROJECT_BASE}/dialogue/PatternCorrelator.cpp
    ${PROJECT_BASE}/dialogue/dialogue.cpp
    ${PROJECT_BASE}/dialogue/pattern.cpp
    ${PROJECT_BASE}/db/db_manager.cpp
    ${PROJECT_BASE}/db/dialogue_repository.cpp
    ${PROJECT_BASE}/db/pattern_repository.cpp
    ${PROJECT_BASE}/db/sentence_repository.cpp
    ${PROJECT_BASE}/db/word_repository.cpp
    ${PROJECT_BASE}/utils/PatternUtils.cpp
    ${PROJECT_BASE}/utils/string_conv.cpp
    ${PROJECT_BASE}/utils/tag_stats.cpp
    ${PROJECT_BASE}/nlp_api.cpp
)

# -----------------------------------------------------------------------
# Ejecutable de pruebas
# -----------------------------------------------------------------------
add_executable(run_tests
    test_tokenizer.cpp
    test_morphology.cpp
    test_classifier.cpp
    test_repositories.cpp
    test_pattern_correlator.cpp
    test_contextual_correlator.cpp
    test_dialogue.cpp
    test_utils.cpp
    test_integration.cpp
    ${PROJECT_SOURCES}
)

target_include_directories(run_tests PRIVATE
    ${PROJECT_BASE}
    ${PROJECT_BASE}/core
    ${PROJECT_BASE}/db
    ${PROJECT_BASE}/dialogue
    ${PROJECT_BASE}/nlp
    ${PROJECT_BASE}/tokenizer
    ${PROJECT_BASE}/utils
)

target_link_libraries(run_tests PRIVATE Catch2::Catch2WithMain SQLite::SQLite3)

# Registrar las pruebas con CTest (usando el módulo de Catch2)
catch_discover_tests(run_tests)

# -----------------------------------------------------------------------
# Configuración para Valgrind
# -----------------------------------------------------------------------
find_program(VALGRIND_EXECUTABLE valgrind)
if(VALGRIND_EXECUTABLE)
    set(CTEST_MEMORYCHECK_COMMAND "${VALGRIND_EXECUTABLE}" CACHE STRING "Command for memory check")
    set(CTEST_MEMORYCHECK_COMMAND_OPTIONS "--leak-check=full --error-exitcode=1" CACHE STRING "Options for memory check")
endif()
