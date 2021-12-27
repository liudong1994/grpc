#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <queue>
#include <iostream>
#include <thread>

#include "tfserving/prediction_service.grpc.pb.h"

#include <grpc++/grpc++.h>
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>

using namespace std;

using tensorflow::serving::PredictRequest;
using tensorflow::serving::PredictResponse;
using tensorflow::serving::PredictionService;
using grpc::Status;
using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;


/*
    思路: 将AsyncClientCall放到req中 结构体, Finish函数中传入req指针
    AsyncCompleteRpc线程完成后 通知网络库协程, 进行恢复操作

    AsyncCompleteRpc只需要一个线程, m_stub还是初始化好很多个连接, 来了请求从连接池里取
*/
struct AsyncClientCall {
    PredictResponse reply;
    ClientContext   context;
    Status status;
    std::unique_ptr<ClientAsyncResponseReader<PredictResponse>> response_reader;
};

class GreeterClient 
{
public:
    GreeterClient(std::shared_ptr<Channel> channel): m_stub(PredictionService::NewStub(channel)) {}

    void AsyncPredict() 
    {
        PredictRequest predictRequest;
        predictRequest.mutable_model_spec()->set_name("imp_ctcvr_rank_model");
        predictRequest.mutable_model_spec()->set_signature_name("serving_default");

        auto& inputs = *(predictRequest.mutable_inputs());

        inputs["creativeid"] = tensorflow::TensorProto();
        auto itrInput = inputs.find("creativeid");
        (itrInput->second).set_dtype(tensorflow::DataType::DT_STRING);
        (itrInput->second).add_string_val("12866");
        (itrInput->second).mutable_tensor_shape()->add_dim()->set_size(1);
        //printf("tfPacket RequestPB:\n%s\n", predictRequest.Utf8DebugString().c_str());

        gpr_timespec dead_line;
        dead_line.tv_sec = 3;
        dead_line.tv_nsec = 0;
        dead_line.clock_type = GPR_TIMESPAN;

        // 异步调用，非阻塞
        AsyncClientCall* call = new AsyncClientCall;
        call->context.set_deadline(dead_line);
        call->response_reader = m_stub->PrepareAsyncPredict(&call->context, predictRequest, &m_completionQueue);
        call->response_reader->StartCall();
        // 当 RPC 调用结束时，让 gRPC 自动将返回结果填充到 AsyncClientCall 中
        // 并将 AsyncClientCall 的地址加入到队列中
        call->response_reader->Finish(&call->reply, &call->status, (void*)call);
    }

    void AsyncCompleteRpc() 
    {
        static int sCount = 0;

        void* got_tag;
        bool ok = false;
        // 从队列中取出 AsyncClientCall 的地址，会阻塞
        while (m_completionQueue.Next(&got_tag, &ok)) 
        {
            AsyncClientCall* call = static_cast<AsyncClientCall*>(got_tag);
            if (call->status.ok())
                std::cout << "PredictionService count: " << sCount++ << ", received:" << call->reply.Utf8DebugString() << std::endl;
            else
                std::cout << "RPC failed, code: " << call->status.error_code() << ", message: " << call->status.error_message().c_str() << std::endl;
			
            delete call;  // 销毁对象 
        }
    }

private:
    std::unique_ptr<PredictionService::Stub> m_stub;
    CompletionQueue m_completionQueue;    // 队列
};

int SyncCall();
int AsyncCall();

int main()
{
    // SyncCall();
    AsyncCall();

    return 0;
}

int AsyncCall()
{
    // grpc发送给tfserving
    grpc::ChannelArguments args;
    args.SetInt(GRPC_ARG_USE_LOCAL_SUBCHANNEL_POOL, 1);
    auto channel = grpc::CreateCustomChannel("172.29.209.181:8666", grpc::InsecureChannelCredentials(), args);

    // async
    // 启动新线程，从队列中取出结果并处理
    GreeterClient greeter(channel);

    for (int i = 0; i < 2; i++) {
        greeter.AsyncPredict();
    }

    sleep(3);

    std::thread thread_ = std::thread(&GreeterClient::AsyncCompleteRpc, &greeter);

    for (int i = 0; i < 2; i++) {
        greeter.AsyncPredict();
    }

    thread_.join();

    return 0;
}

int SyncCall()
{
    // pb构造 发送
    PredictRequest predictRequest;
    PredictResponse predictResponse;

    predictRequest.mutable_model_spec()->set_name("imp_ctcvr_rank_model");
    predictRequest.mutable_model_spec()->set_signature_name("serving_default");

    auto& inputs = *(predictRequest.mutable_inputs());

    inputs["creativeid"] = tensorflow::TensorProto();
    auto itrInput = inputs.find("creativeid");
    (itrInput->second).set_dtype(tensorflow::DataType::DT_STRING);
    (itrInput->second).add_string_val("12866");
    (itrInput->second).add_string_val("12866");
    (itrInput->second).mutable_tensor_shape()->add_dim()->set_size(2);

    printf("tfPacket RequestPB:\n%s\n", predictRequest.Utf8DebugString().c_str());

    // grpc发送给tfserving
    grpc::ChannelArguments args;
    args.SetInt(GRPC_ARG_USE_LOCAL_SUBCHANNEL_POOL, 1);
    auto channel = grpc::CreateCustomChannel("172.29.209.181:8666", grpc::InsecureChannelCredentials(), args);
    auto stub = PredictionService::NewStub(channel);

    gpr_timespec timespec;
    timespec.tv_sec = 3;
    timespec.tv_nsec = 0;
    timespec.clock_type = GPR_TIMESPAN;

    ClientContext context;
    context.set_deadline(timespec);
    Status status = stub->Predict(&context, predictRequest, &predictResponse);

    printf("grpc error code:%d  message:%s\n", status.error_code(), status.error_message().c_str());
    printf("tfParse ResponsePB:\n%s", predictResponse.Utf8DebugString().c_str());

    google::protobuf::Map<std::string, tensorflow::TensorProto>& mapOutputs = *predictResponse.mutable_outputs();
    auto itrOutput = mapOutputs.find("cvr");
    if (itrOutput != mapOutputs.end()) {
        tensorflow::TensorProto& protoResultTensor = itrOutput->second;
        if (protoResultTensor.double_val_size() > 0) {
            printf("ResponsePB score: %f\n", protoResultTensor.double_val(0));
        }
    }

    return 0;
}

/*
    grpc version: 1.38.1
    wrappers.proto需要是google/protobuf/wrappers.proto, 否则全局冲突
    cd tfserving
    protoc *.proto --cpp_out=./
    protoc -I . --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ./prediction_service.proto
*/

