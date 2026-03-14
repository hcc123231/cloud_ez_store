#pragma once

#include "muduo/net/TcpServer.h"
#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

#include <functional>

namespace muduo {
namespace net {

class HttpServer {
public:
    using HttpCallback = std::function<bool (const TcpConnectionPtr&, HttpRequest&, HttpResponse*)>;

    HttpServer(EventLoop* loop,
              const InetAddress& listenAddr,
              const std::string& name);

    void setHttpCallback(const HttpCallback& cb) { httpCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback& cb) { server_.setConnectionCallback(cb); }
    void setThreadNum(int numThreads) { server_.setThreadNum(numThreads); }
    void start() { server_.start(); }

private:
    void onConnection(const TcpConnectionPtr& conn);
    //用户消息回调出现位置
    //onMessage本身当检测到fd可读时回调
    void onMessage(const TcpConnectionPtr& conn,
                  Buffer* buf,
                  Timestamp receiveTime);
    //用户消息回调出现位置
    //onRequest本身在onMessage中当整个请求解析完成时调用
    bool onRequest(const TcpConnectionPtr&, HttpRequest&);

    /*
    所以最后得出结论：用户消息回调也就是httpCallback_在onMessage中头部解析完成时回调或者整体解析完成时回调
    */

    TcpServer server_;
    HttpCallback httpCallback_;
}; // class HttpServer

} // namespace net
} // namespace muduo 