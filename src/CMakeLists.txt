set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(LIB_SRC
    util/print.hpp
    util/tuple_visit.hpp
    util/util.hpp

    type.h
    opcode.h
    instruction.h
    constant.h
    function.h
    exception.h

    file.h
    file.cpp

    vm.h
    vm.cpp
)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} argparse LIB_SRC)