add_executable(simple_server simple_server.cc simple.pb.cc)
target_link_libraries(simple_server protobuf_rpc kanon_base kanon_net)

add_executable(simple_client simple_client.cc simple.pb.cc)
target_link_libraries(simple_client protobuf_rpc kanon_base kanon_net)

add_executable(simple_client2 simple_client2.cc simple.pb.cc)
target_link_libraries(simple_client2 protobuf_rpc kanon_base kanon_net)

add_executable(simple_client3 simple_client3.cc simple.pb.cc)
target_link_libraries(simple_client3 protobuf_rpc kanon_base kanon_net)