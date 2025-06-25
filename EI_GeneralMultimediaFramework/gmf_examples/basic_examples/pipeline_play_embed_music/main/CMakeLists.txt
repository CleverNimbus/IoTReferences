include( ${PROJECT_DIR}/main/esp_embed_tone.cmake )
idf_component_register(SRC_DIRS "./"
                       INCLUDE_DIRS "./"
                       EMBED_FILES ${COMPONENT_EMBED_TXTFILES})
