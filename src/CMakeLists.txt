add_subdirectory(eigenutils)
add_subdirectory(functionapproximators)
add_subdirectory(dynamicalsystems)
add_subdirectory(dmp)

install(
    EXPORT dmpbbo
    DESTINATION lib/cmake/dmpbbo
    NAMESPACE dmpbbo::
    FILE dmpbboConfig.cmake)