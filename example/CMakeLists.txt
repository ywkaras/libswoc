cmake_minimum_required(VERSION 3.12)
project(libswoc_examples CXX)
set(CMAKE_CXX_STANDARD 17)

add_executable(ex_netdb ex_netdb.cc)
target_link_libraries(ex_netdb PUBLIC libswoc)
set_target_properties(ex_netdb PROPERTIES CLANG_FORMAT_DIRS ${CMAKE_CURRENT_SOURCE_DIR})

if (CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(ex_netdb PRIVATE -Wall -Wextra -Werror)
endif()

add_executable(ex_netcompact ex_netcompact.cc)
target_link_libraries(ex_netcompact PUBLIC libswoc)
if (CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(ex_netcompact PRIVATE -Wall -Wextra -Werror)
endif()

add_executable(ex_flat_space ex_flat_space.cc)
target_link_libraries(ex_flat_space PUBLIC libswoc)
if (CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(ex_flat_space PRIVATE -Wall -Wextra -Werror)
endif()

add_executable(ex_diskstats ex_diskstats.cc)
target_link_libraries(ex_diskstats PUBLIC libswoc)
if (CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(ex_diskstats PRIVATE -Wall -Wextra -Werror)
endif()

add_executable(ex_lru_cache ex_lru_cache.cc)
target_link_libraries(ex_lru_cache PUBLIC libswoc)
if (CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(ex_lru_cache PRIVATE -Wall -Wextra -Werror)
    target_link_options(ex_lru_cache PRIVATE -lpthread)
endif()
