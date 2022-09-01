# Installation
## Google Protobuf
protobuf不同版本之间可能不太兼容，因此安装正确的版本的protobuf是很有必要的。
```shell
wget https://github.com/protocolbuffers/protobuf/releases/download/v21.5/protobuf-cpp-3.21.5.tar.gz
tar xf protobuf-cpp-3.21.5.tar.gz

cd protobuf-3.21.5
cmake . -DCMAKE_BUILD_TYPE=Release \
-Dprotobuf_BUILD_TESTS=OFF \
-Dprotobuf_BUILD_SHARED_LIBS=ON \
-DCMAKE_INSTALL_PREFIX=/usr
cmake --build . --parallel 2
cmake --install .
```

## Protobuf
kanon对于protobuf和rpc两个模块的编译都是可选项。
如果你想编译它们，命令如下：
```shell
cd kanon/build
# 其他选项通过cmake .. -L查看
cmake .. -DBUILD_PROTOBUF=ON -DBUILD_PROTOBUF_RPC=ON ..
```