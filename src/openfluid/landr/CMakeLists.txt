MACRO(OPENFLUID_ADD_GEOS_DEFINITIONS)
  STRING(COMPARE LESS ${GEOS_VERSION} "3.3.0" GEOS_VERSION_LESS_THAN_3_3_0)
  IF(GEOS_VERSION_LESS_THAN_3_3_0)
    SET(GEOS_VERSION_GREATER_OR_EQUAL_3_3_0  0)
  ELSE()
    SET(GEOS_VERSION_GREATER_OR_EQUAL_3_3_0  1)
  ENDIF()

  STRING(COMPARE LESS ${GEOS_VERSION} "3.3.2" GEOS_VERSION_LESS_THAN_3_3_2)
  IF(GEOS_VERSION_LESS_THAN_3_3_2)
    SET(GEOS_VERSION_GREATER_OR_EQUAL_3_3_2  0)
  ELSE()
    SET(GEOS_VERSION_GREATER_OR_EQUAL_3_3_2  1)
  ENDIF()
  
  ADD_DEFINITIONS(-DGEOS_VERSION_GREATER_OR_EQUAL_3_3_0=${GEOS_VERSION_GREATER_OR_EQUAL_3_3_0} -DGEOS_VERSION_GREATER_OR_EQUAL_3_3_2=${GEOS_VERSION_GREATER_OR_EQUAL_3_3_2})
ENDMACRO()


######################


SET(LANDR_CPP LandREntity.cpp LineStringEntity.cpp  PolygonEntity.cpp
              PolygonEdge.cpp
              LandRGraph.cpp PolygonGraph.cpp LineStringGraph.cpp
              VectorDataset.cpp RasterDataset.cpp
              LandRTools.cpp
              GEOSHelpers.cpp
              )

SET(LANDR_HPP LandREntity.hpp LineStringEntity.hpp PolygonEntity.hpp
              PolygonEdge.hpp
              LandRGraph.hpp PolygonGraph.hpp LineStringGraph.hpp
              VectorDataset.hpp RasterDataset.hpp
              LandRTools.hpp
              GEOSHelpers.hpp
              )


FIND_PACKAGE(GEOS REQUIRED)
FIND_PACKAGE(GDAL REQUIRED)

OPENFLUID_ADD_GEOS_DEFINITIONS()

INCLUDE_DIRECTORIES(${GEOS_INCLUDE_DIR} ${GDAL_INCLUDE_DIR})


ADD_LIBRARY(openfluid-landr SHARED ${LANDR_CPP} ${LANDR_HPP})

SET_TARGET_PROPERTIES(openfluid-landr PROPERTIES VERSION "${OPENFLUID_VERSION_MAJOR}.${OPENFLUID_VERSION_MINOR}" 
                                        SOVERSION "${OPENFLUID_VERSION_MAJOR}.${OPENFLUID_VERSION_MINOR}.${OPENFLUID_VERSION_PATCH}"
                                        LIBRARY_OUTPUT_DIRECTORY "${OFLANDR_DIST_LIB_PATH}"
                                        RUNTIME_OUTPUT_DIRECTORY "${OFLANDR_DIST_LIB_PATH}"
                                        ARCHIVE_OUTPUT_DIRECTORY "${OFLANDR_DIST_LIB_PATH}"
                                        COMPILE_DEFINITIONS "OPENFLUID_DLL"
                                        DEFINE_SYMBOL "OPENFLUID_DLL_EXPORTS")

TARGET_LINK_LIBRARIES(openfluid-landr
                      ${OpenFLUID_LIBRARIES}
                      ${GDAL_LIBRARIES} ${GEOS_LIBRARY})


INSTALL(TARGETS openfluid-landr
        RUNTIME DESTINATION ${OFLANDR_INSTALL_LIB_PATH}
        LIBRARY DESTINATION ${OFLANDR_INSTALL_LIB_PATH}
        ARCHIVE DESTINATION ${OFLANDR_INSTALL_LIB_PATH})
   
INSTALL(FILES ${LANDR_HPP} DESTINATION ${OFLANDR_INSTALL_INCLUDE_PATH})


IF(OFLANDR_ENABLE_TESTING)
  ADD_SUBDIRECTORY(tests)
ENDIF()
