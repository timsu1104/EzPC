add_library(SCI-GC
    emp-tool.cpp
    halfgate_gen.cpp
    halfgate_eva.cpp
    number.cpp
    orcompact.cpp
    deduplicate.cpp
    lowmc.cpp
    subcube_query.cpp)
target_link_libraries(SCI-GC
    PUBLIC SCI-utils SCI-OT
)
