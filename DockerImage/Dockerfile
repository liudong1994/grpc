FROM centos:8.4.2105

RUN yum install -y cmake gcc gcc-c++ \ 
	&& yum install -y which

COPY ./pkg /home/pkg

RUN cd /home/pkg/grpc-1.38.1/cmake && mkdir build && cd build && cmake ../.. && make -j6 && make install \
	&& cd /home/pkg/grpc-1.38.1/third_party/abseil-cpp/ && mkdir build && cd build && cmake .. && make -j6 && make install

