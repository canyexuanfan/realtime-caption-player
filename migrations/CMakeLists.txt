# Migrations: SQL schema files consumed by the storage layer at runtime.
# Installed as content; no compile targets required for the pure-logic build.
if(NOT CMAKE_CONFIGURATION_TYPES)
    set(_cfg "${CMAKE_BUILD_TYPE}")
else()
    set(_cfg "$<CONFIG>")
endif()

# Copy migration SQL into the build tree so tests/runtime can locate them.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/001_initial.sql
    ${CMAKE_CURRENT_BINARY_DIR}/001_initial.sql
    COPYONLY
)
install(FILES 001_initial.sql DESTINATION migrations)
