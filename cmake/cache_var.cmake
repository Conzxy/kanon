# I don't want to use the builtin variable ${BUILD_SHARED_LIBS}
# since it you don't set it to ON explicitly, then the NOT 
# ${BUILD_SHARED_LIBS} will be false, and generated libraries 
# are static. That's not I wanted behavior. I want a variable,
# it can build shared libraries default even though I don't set 
# it explitly, and the build of static libraries is an option.
option(KANON_BUILD_STATIC_LIBS "Build kanon static libraries" OFF)

# After enable this option and build, you should type `cmake --install`
# to install files to proper destination actually.
option(KANON_INSTALL "Generate the install target" ${KANON_MAIN_PROJECT})

option(KANON_TESTS "Generate kanon test targets" OFF)
option(KANON_EXAMPLES "Generate kanon example targets" OFF)

# User can determine whether to build all tests when build target all
# e.g. cmake --build */kanon/build [--target all -j 2]
# If this option is OFF, user should specify target manually.
option(KANON_BUILD_ALL_TESTS "Build tests when --target all(default) is specified" OFF)
option(KANON_BUILD_ALL_EXAMPLES "Build examples when --target all(default) is specified" OFF)

option(KANON_BUILD_PERF_TEST "Build performance test file" OFF)

option(KANON_BUILD_PROTOBUF "Build protobuf module" OFF)
option(KANON_BUILD_PROTOBUF_RPC "Build protobuf-rpc module" OFF)
