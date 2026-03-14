#ifndef _HTTP_PARSER_WRAPPER_H_
#define _HTTP_PARSER_WRAPPER_H_

#include "http_parser.h"
#include <string>

class CHttpParserWrapper {
public:
    CHttpParserWrapper();
    ~CHttpParserWrapper() = default;

    // 解析新收到的数据，支持分片。返回已解析的字节数，出错返回0
    size_t Parse(const char* data, size_t len);

    // 重置解析器状态，准备解析下一个请求（用于 keep-alive）
    void Reset();

    // 判断是否已完成一个完整的HTTP请求解析
    bool IsCompleted() const { return completed_; }

    // 获取解析结果
    const std::string& GetUrl() const { return url_; }
    const std::string& GetBody() const { return body_; }
    const std::string& GetReferer() const { return referer_; }
    const std::string& GetForwardIP() const { return forward_ip_; }
    const std::string& GetUserAgent() const { return user_agent_; }
    const std::string& GetContentType() const { return content_type_; }
    uint32_t GetContentLength() const { return content_length_; }
    const std::string& GetHost() const { return host_; }
    uint8_t GetMethod() const { return method_; }  // HTTP method 枚举值

    // 为兼容旧代码保留的接口（建议逐步替换）
    char* GetUrl() { return (char*)url_.c_str(); }
    char* GetBodyContent() { return (char*)body_.c_str(); }
    uint32_t GetBodyContentLen() { return (uint32_t)body_.length(); }
    bool IsReadAll() { return completed_; }

private:
    // 回调函数（静态）
    static int OnUrl(http_parser* parser, const char* at, size_t length, void* obj);
    static int OnHeaderField(http_parser* parser, const char* at, size_t length, void* obj);
    static int OnHeaderValue(http_parser* parser, const char* at, size_t length, void* obj);
    static int OnHeadersComplete(http_parser* parser, void* obj);
    static int OnBody(http_parser* parser, const char* at, size_t length, void* obj);
    static int OnMessageComplete(http_parser* parser, void* obj);

    // 辅助设置方法
    void SetUrl(const char* at, size_t len) { url_.append(at, len); }
    void SetBody(const char* at, size_t len) { body_.append(at, len); }
    void SetReferer(const char* at, size_t len) { referer_.append(at, len); }
    void SetForwardIP(const char* at, size_t len) { forward_ip_.append(at, len); }
    void SetUserAgent(const char* at, size_t len) { user_agent_.append(at, len); }
    void SetContentType(const char* at, size_t len) { content_type_.append(at, len); }
    void SetContentLength(uint32_t len) { content_length_ = len; }
    void SetHost(const char* at, size_t len) { host_.append(at, len); }
    void SetMethod(uint8_t method) { method_ = method; }
    void SetCompleted() { completed_ = true; }

    enum class Field {
        NONE,
        REFERER,
        FORWARD_IP,
        USER_AGENT,
        CONTENT_TYPE,
        CONTENT_LEN,
        HOST
    } current_field_;

    http_parser parser_;
    http_parser_settings settings_;

    bool completed_;
    std::string url_;
    std::string body_;
    std::string referer_;
    std::string forward_ip_;
    std::string user_agent_;
    std::string content_type_;
    uint32_t content_length_;
    std::string host_;
    uint8_t method_;
};

#endif