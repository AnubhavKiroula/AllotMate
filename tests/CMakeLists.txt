# Test targets for the Member 3 database layer.
# Both link the shared core library and inherit its include paths / libpqxx link.

# Pure-logic tests: CSV validation + capacity invariant. No database required,
# so this always runs (including in CI).
add_executable(test_csv_importer test_csv_importer.cpp)
target_link_libraries(test_csv_importer PRIVATE allotmate_core)
target_compile_options(test_csv_importer PRIVATE ${ALLOTMATE_WARNINGS})
add_test(NAME csv_importer COMMAND test_csv_importer)

# Integration tests: repositories, run log, pool. Skip themselves (exit 0) when
# no database is reachable. Run from the source dir so a local .env is found.
add_executable(test_db_layer test_db_layer.cpp)
target_link_libraries(test_db_layer PRIVATE allotmate_core)
target_compile_options(test_db_layer PRIVATE ${ALLOTMATE_WARNINGS})
add_test(NAME db_layer COMMAND test_db_layer)
set_tests_properties(db_layer PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
