# Resource defaults, icons, model manifests and translations.
# A Qt resource collection (.qrc) is added in the UI phase (P7).
# For now copy the default config files into the build tree so that
# tests and the runtime can locate them.

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/defaults
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# models/ holds the model bundle manifest (schema only; weights are fetched
# later per T0013 and never committed). Expose the manifest path.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/models/model-bundles.json
    ${CMAKE_CURRENT_BINARY_DIR}/models/model-bundles.json
    COPYONLY
)
