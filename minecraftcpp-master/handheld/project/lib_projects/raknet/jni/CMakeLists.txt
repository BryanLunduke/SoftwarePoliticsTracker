FILE(GLOB raknetCPPSources ./RaknetSources/*.cpp)
set(CMAKE_C_FLAGS -m32)
set(CMAKE_CXX_FLAGS -m32)
add_library(raknet ${raknetCPPSources})
