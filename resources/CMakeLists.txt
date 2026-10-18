# Resource defaults, icons, model manifests and translations.
# A Qt resource collection (.qrc) is added in the UI phase (P7).
# For now copy the default config files into the build tree so that
# tests and the runtime can locate them.

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/defaults
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# models/ holds the model bundle manifest (schema only; weights are fetched
# later per T0013 and never committed). Expose the manifest path.
# The manifest is generated locally by tools/fetch-models (A3 gating) and is
# gitignored; on CI / fresh clones it does not exist, so fall back to an
# empty-bundle placeholder instead of failing configure (2026-10-18 CI fix).
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/models/model-bundles.json)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/models/model-bundles.json
        ${CMAKE_CURRENT_BINARY_DIR}/models/model-bundles.json
        COPYONLY
    )
else()
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/models/model-bundles.json
"{\n  \"schemaVersion\": 1,\n  \"generatedBy\": \"cmake-fallback\",\n  \"bundles\": []\n}\n")
    message(STATUS "resources: model-bundles.json not present (fresh clone/CI); wrote empty placeholder")
endif()
