add_library(http-server STATIC
  header.h
  request.cc
  request.h
  response.h
  response.cc
  router.h
  router.cc
  server.cc
  server.h
  utils.cc
  utils.h
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
        header.h
        request.h
        response.h
        router.h
        server.h
        utils.h
  )

  install(TARGETS http-server
    ARCHIVE DESTINATION lib/http-server
    FILE_SET public_headers DESTINATION include/http-server
  )
endif()