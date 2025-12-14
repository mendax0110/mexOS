# CMakeLists.txt for servers/lib
# Common library for user-space servers

set(SERVERS_LIB_SOURCES
        io_port.c
        ipc_client.c
        block_client.c
        memory.c
)

set(SERVERS_LIB_HEADERS
        io_port.h
        ipc_client.h
        block_client.h
        memory.h
)

# Note: This library is compiled as part of each server executable,
# not as a separate library, since each server needs its own instance.
# The sources are included via the parent CMakeLists.txt

# Export source list for parent CMakeLists.txt
set(SERVERS_LIB_SOURCES ${SERVERS_LIB_SOURCES} PARENT_SCOPE)
