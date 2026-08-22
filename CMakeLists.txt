cmake_minimum_required(VERSION 3.16)

project(AllotMate
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find PostgreSQL package
find_package(PostgreSQL REQUIRED)

# Find libpqxx (first look for CMake config, then standard paths)
find_package(libpqxx CONFIG QUIET)
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
    src/PostgresConnection.cpp
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