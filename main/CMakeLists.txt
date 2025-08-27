include(${CMAKE_CURRENT_LIST_DIR}/CONFIG.cmake)

idf_component_register(
    SRCS "main.cpp"
        ${SOURCES}
    INCLUDE_DIRS "."
    REQUIRES
)
