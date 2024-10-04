set(srcs
    wn_perf_tester.cpp
    mn_perf_tester.cpp
    perf_tester_cmd.cpp
    )

set(requires
    hardware_driver
    esp-sr
    player
    console
    )

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "."
    REQUIRES ${requires})

