# grpc centos8 compile

centos8 grpc镜像，以及编译示例

```shell
git clone https://github.com/liudong1994/grpc-code.git

cd grpc-code/DockerImage/
# build docker centos 8
docker build -t grpc-build-test .
# GIT_SRC_PATH替换为当前git下载目录
docker run --network=host --name grpc-build-test -itd -v $GIT_SRC_PATH:/home/grpc grpc-build-test
docker exec -it grpc-build-test /bin/bash

# 编译示例
cd /home/grpc/grpc-code/tfserving
protoc *.proto --cpp_out=./
protoc -I . --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ./prediction_service.proto

cd /home/grpc/grpc-code
make mkdir && make

# 执行
./bin/Test
```



