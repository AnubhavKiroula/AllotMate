cmake_minimum_required(VERSION 3.16)

project(AllotMate
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find PostgreSQL (required for libpq)
find_package(PostgreSQL REQUIRED)

# 1. Try finding libpqxx via CMake Config (e.g. vcpkg on Windows or distro CMake package)
find_package(libpqxx CONFIG QUIET)

# 2. Try finding via PkgConfig (standard on Linux / Ubuntu)
if(NOT TARGET libpqxx::pqxx)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(PQXX QUIET IMPORTED_TARGET libpqxx)
        if(TARGET PkgConfig::PQXX)
            add_library(libpqxx::pqxx ALIAS PkgConfig::PQXX)
        endif()
    endif()
endif()

# 3. Fallback: manual path discovery
if(NOT TARGET libpqxx::pqxx)
    find_path(LIBPQXX_INCLUDE_DIR pqxx/pqxx
        HINTS
            ${PostgreSQL_INCLUDE_DIRS}
            /usr/include
            /usr/local/include
    )
    find_library(LIBPQXX_LIBRARY NAMES pqxx libpqxx
        HINTS
            ${PostgreSQL_LIBRARY_DIRS}
            /usr/lib
            /usr/lib/x86_64-linux-gnu
            /usr/lib/aarch64-linux-gnu
            /usr/local/lib
    )
    if(LIBPQXX_INCLUDE_DIR AND LIBPQXX_LIBRARY)
        add_library(libpqxx::pqxx UNKNOWN IMPORTED)
        set_target_properties(libpqxx::pqxx PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${LIBPQXX_INCLUDE_DIR}"
            IMPORTED_LOCATION "${LIBPQXX_LIBRARY}"
        )
    endif()
endif()

add_executable(AllotMate
    src/main.cpp
    src/CLI.cpp
    src/PostgresConnection.cpp
    src/Student.cpp
    src/Coordinator.cpp
    src/Elective.cpp
    src/MatchingEngine.cpp
)

target_include_directories(AllotMate
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PostgreSQL_INCLUDE_DIRS}
)

if(TARGET libpqxx::pqxx)
    target_link_libraries(AllotMate PRIVATE libpqxx::pqxx)
elseif(LIBPQXX_LIBRARY)
    target_link_libraries(AllotMate PRIVATE ${LIBPQXX_LIBRARY})
else()
    # Final fallback: direct link for apt-installed libpqxx on Ubuntu/Debian
    # (no CMake config file is shipped with the apt package)
    target_link_libraries(AllotMate PRIVATE pqxx pq)
endif()

if(TARGET PostgreSQL::PostgreSQL)
    target_link_libraries(AllotMate PRIVATE PostgreSQL::PostgreSQL)
elseif(PostgreSQL_LIBRARIES)
    target_link_libraries(AllotMate PRIVATE ${PostgreSQL_LIBRARIES})
endif()

if(MSVC)
    target_compile_options(AllotMate PRIVATE /W4)
else()
    target_compile_options(AllotMate PRIVATE -Wall -Wextra -Werror)
endif()