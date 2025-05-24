FetchContent_Declare(
    tracy
    GIT_REPOSITORY  https://github.com/wolfpld/tracy.git
    GIT_TAG         v0.11.1
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)
set(TRACY_ENABLE ON CACHE BOOL "Enable profiling")
set(TRACY_ONLY_IPV4 ON CACHE BOOL "IPv4 only")
#set(TRACY_NO_SYSTEM_TRACING ON CACHE BOOL "Disable System Tracing")
message("Fetching tracy")
FetchContent_MakeAvailable(tracy)
