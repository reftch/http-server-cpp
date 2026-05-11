add_library(http-server STATIC
  header.hpp
  request.cpp
  request.hpp
  response.hpp
  response.cpp
  router.hpp
  router.cpp
  server.cpp
  server.hpp
  utils.cpp
  utils.hpp
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_include_directories(http-server
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  target_sources(http-server
    PUBLIC
      FILE_SET public_headers
      TYPE HEADERS
      FILES 
        server.hpp
  )

  install(TARGETS http-server
    ARCHIVE DESTINATION lib/http-server
    FILE_SET public_headers DESTINATION include/http-server
  )
endif()