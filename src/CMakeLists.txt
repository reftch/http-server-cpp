add_library(http-server STATIC
  client.h
  client.cc
  logger.h
  request.cc
  request.h
  response.h
  response.cc
  router.h
  router.cc
  server.cc
  server.h
  sslserver.h
  utils.cc
  utils.h
  websocket.h
  websocket.cc
  websocketclient.h
)

target_include_directories(http-server
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)