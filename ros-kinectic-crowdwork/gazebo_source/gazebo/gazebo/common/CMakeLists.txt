include (${gazebo_cmake_dir}/GazeboUtils.cmake)

add_definitions(-DBUILDING_DLL_GZ_COMMON)

# tiny obj loader
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/deps/tinyobjloader)

if (HAVE_OPENAL)
  include_directories(${OPENAL_INCLUDE_DIR})
endif()

if (HAVE_GTS)
  include_directories(${gts_INCLUDE_DIRS})
  link_directories(${gts_LIBRARY_DIRS})
  add_definitions(${gts_CFLAGS})
endif()

if (HAVE_GDAL)
  include_directories(${GDAL_INCLUDE_DIR})
endif()

if (CURL_FOUND)
  include_directories(${CURL_INCLUDEDIR})
  link_directories(${CURL_LIBDIR})
  if (WIN32)
    add_definitions(-DCURL_STATICLIB)
  endif()
endif()

include_directories(${tinyxml_INCLUDE_DIRS})
link_directories(${tinyxml_LIBRARY_DIRS})

set (sources
  Animation.cc
  Assert.cc
  AudioDecoder.cc
  Battery.cc
  Base64.cc
  BVHLoader.cc
  ColladaExporter.cc
  ColladaLoader.cc
  Color.cc
  CommonIface.cc
  Console.cc
  Dem.cc
  Event.cc
  Events.cc
  Exception.cc
  HeightmapData.cc
  Image.cc
  ImageHeightmap.cc
  KeyEvent.cc
  KeyFrame.cc
  Material.cc
  MaterialDensity.cc
  Mesh.cc
  MeshExporter.cc
  MeshLoader.cc
  MeshManager.cc
  ModelDatabase.cc
  MouseEvent.cc
  OBJLoader.cc
  PID.cc
  SemanticVersion.cc
  SkeletonAnimation.cc
  Skeleton.cc
  SphericalCoordinates.cc
  STLLoader.cc
  SystemPaths.cc
  SVGLoader.cc
  Time.cc
  Timer.cc
  URI.cc
  Video.cc
  VideoEncoder.cc
  ffmpeg_inc.cc
)

if (NOT USE_EXTERNAL_TINYXML)
  include_directories (${CMAKE_SOURCE_DIR}/deps/win/tinyxml)
  set (sources ${sources}
       ${CMAKE_SOURCE_DIR}/deps/win/tinyxml/tinystr.cpp
       ${CMAKE_SOURCE_DIR}/deps/win/tinyxml/tinyxml.cpp
       ${CMAKE_SOURCE_DIR}/deps/win/tinyxml/tinyxmlerror.cpp
       ${CMAKE_SOURCE_DIR}/deps/win/tinyxml/tinyxmlparser.cpp)
endif()

set (headers
  Animation.hh
  Assert.hh
  AudioDecoder.hh
  Battery.hh
  Base64.hh
  BVHLoader.hh
  ColladaLoader.hh
  CommonIface.hh
  CommonTypes.hh
  Color.hh
  Console.hh
  Dem.hh
  EnumIface.hh
  Event.hh
  Events.hh
  Exception.hh
  MovingWindowFilter.hh
  HeightmapData.hh
  Image.hh
  ImageHeightmap.hh
  KeyEvent.hh
  KeyFrame.hh
  Material.hh
  MaterialDensity.hh
  Mesh.hh
  MeshLoader.hh
  MeshManager.hh
  ModelDatabase.hh
  MouseEvent.hh
  OBJLoader.hh
  PID.hh
  Plugin.hh
  SemanticVersion.hh
  SkeletonAnimation.hh
  Skeleton.hh
  SingletonT.hh
  SphericalCoordinates.hh
  STLLoader.hh
  SystemPaths.hh
  SVGLoader.hh
  Time.hh
  Timer.hh
  UpdateInfo.hh
  URI.hh
  Video.hh
  VideoEncoder.hh
  ffmpeg_inc.h
 )

set (gtest_sources
  Animation_TEST.cc
  Battery_TEST.cc
  ColladaExporter_TEST.cc
  ColladaLoader_TEST.cc
  Color_TEST.cc
  CommonIface_TEST.cc
  Console_TEST.cc
  Dem_TEST.cc
  EnumIface_TEST.cc
  Exception_TEST.cc
  Event_TEST.cc
  HeightmapData_TEST.cc
  Image_TEST.cc
  ImageHeightmap_TEST.cc
  Material_TEST.cc
  MaterialDensity_TEST.cc
  Mesh_TEST.cc
  MeshManager_TEST.cc
  MouseEvent_TEST.cc
  MovingWindowFilter_TEST.cc
  OBJLoader_TEST.cc
  SemanticVersion_TEST.cc
  SphericalCoordinates_TEST.cc
  SystemPaths_TEST.cc
  SVGLoader_TEST.cc
  Time_TEST.cc
  URI_TEST.cc
  VideoEncoder_TEST.cc
)

# Timer test fails on OSX
# See issue #1626
if (NOT APPLE)
  set (gtest_sources
    ${gtest_sources}
    Timer_TEST.cc
  )
endif()

if (HAVE_GTS)
set (sources
  ${sources}
  GTSMeshUtils.cc
  MeshCSG.cc
)
set (headers
  ${headers}
  GTSMeshUtils.hh
  MeshCSG.hh
)

set (gtest_sources
  ${gtest_sources}
  GTSMeshUtils_TEST.cc
)
endif()

if (HAVE_FFMPEG)
  set (gtest_sources ${gtest_sources}
                     AudioDecoder_TEST.cc)
endif()

gz_build_tests(${gtest_sources} EXTRA_LIBS gazebo_common)

set (common_headers "" CACHE INTERNAL "common headers" FORCE)
foreach (hdr ${headers})
  APPEND_TO_CACHED_STRING(common_headers
    "Common Headers" "#include \"gazebo/common/${hdr}\"\n")
endforeach()
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/common.hh.in ${CMAKE_CURRENT_BINARY_DIR}/common.hh )

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBUILDING_DLL_GZ_COMMON")
if (WIN32)
  # 0x601 = Windows 7.  TBB needs to know this.
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x601")
endif ()

gz_add_library(gazebo_common ${sources})

set_property(
  SOURCE SystemPaths.cc SystemPaths_TEST.cc ModelDatabase.cc Console.cc
  PROPERTY COMPILE_DEFINITIONS
  GAZEBO_PLUGIN_PATH="${GAZEBO_PLUGIN_PATH}"
  GAZEBO_MODEL_PATH="${GAZEBO_MODEL_PATH}"
  GAZEBO_RESOURCE_PATH="${GAZEBO_RESOURCE_PATH}"
  GAZEBO_MODEL_DATABASE_URI="${GAZEBO_MODEL_DATABASE_URI}"
  GAZEBO_DEFAULT_MASTER_PORT=${GAZEBO_DEFAULT_MASTER_PORT}
  OGRE_RESOURCE_PATH="${OGRE_RESOURCE_PATH}"
)

target_link_libraries(gazebo_common
  gazebo_math
  ${IGNITION-MATH_LIBRARIES}
  ${libdl_library}
  ${libtool_library}
  ${Boost_LIBRARIES}
  ${freeimage_LIBRARIES}
  ${tinyxml_LIBRARIES}
  ${libavcodec_LIBRARIES}
  ${libavformat_LIBRARIES}
  ${libavutil_LIBRARIES}
  ${libavdevice_LIBRARIES}
  ${CURL_LIBRARIES}
  ${libswscale_LIBRARIES}
  ${libtar_LIBRARIES}
  ${TBB_LIBRARIES}
  ${SDFormat_LIBRARIES}
)

if (UNIX)
  target_link_libraries(gazebo_common pthread)
endif()

if (UNIX AND NOT APPLE)
  # rt is used for clock_gettime, which is not available on apple or windows
  target_link_libraries(gazebo_common rt)
endif()

if (HAVE_GTS)
  target_link_libraries(gazebo_common ${gts_LIBRARIES})
endif()

if (HAVE_OPENAL)
  target_link_libraries(gazebo_common ${OPENAL_LIBRARY})
endif()

if (HAVE_GDAL)
  target_link_libraries(gazebo_common ${GDAL_LIBRARY})
endif()

if (USE_PCH)
  set(MORE_FLAGS "-pthread")
  add_pch(gazebo_common common_pch.hh ${MORE_FLAGS})
endif()


gz_install_library(gazebo_common)
gz_install_includes("common" ${headers} ${CMAKE_CURRENT_BINARY_DIR}/common.hh)
