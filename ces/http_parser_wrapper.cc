#include "http_parser_wrapper.h"
#define MAX_REFERER_LEN 32
#include <strings.h>
CHttpParserWrapper::CHttpParserWrapper()
    : current_field_(Field::NONE)
    , completed_(false)
    , content_length_(0)
    , method_(0)
{
    http_parser_init(&parser_, HTTP_REQUEST);
    settings_.on_url = OnUrl;
    settings_.on_header_field = OnHeaderField;
    settings_.on_header_value = OnHeaderValue;
    settings_.on_headers_complete = OnHeadersComplete;
    settings_.on_body = OnBody;
    settings_.on_message_complete = OnMessageComplete;
    settings_.object = this;        // 供回调使用
    parser_.data = this;             // 也可通过 parser->data 获取
}

size_t CHttpParserWrapper::Parse(const char* data, size_t len)
{
    if (completed_) {
        // 如果已经完成，需先 Reset 再解析下一个请求
        return 0;
    }
    size_t nparsed = http_parser_execute(&parser_, &settings_, data, len);
    if (parser_.http_errno != 0) {
        // 解析出错，可记录日志
        return 0;
    }
    return nparsed;
}

void CHttpParserWrapper::Reset()
{
    http_parser_init(&parser_, HTTP_REQUEST);
    parser_.data = this;
    current_field_ = Field::NONE;
    completed_ = false;
    url_.clear();
    body_.clear();
    referer_.clear();
    forward_ip_.clear();
    user_agent_.clear();
    content_type_.clear();
    content_length_ = 0;
    host_.clear();
    method_ = 0;
}

int CHttpParserWrapper::OnUrl(http_parser* parser, const char* at, size_t length, void* obj)
{
    auto self = static_cast<CHttpParserWrapper*>(obj);
    self->SetUrl(at, length);
    return 0;
}

int CHttpParserWrapper::OnHeaderField(http_parser* parser, const char* at, size_t length, void* obj)
{
    auto self = static_cast<CHttpParserWrapper*>(obj);
    // 根据字段名设置当前正在读取的字段
    if (strncasecmp(at, "Referer", 7) == 0) {
        self->current_field_ = Field::REFERER;
    } else if (strncasecmp(at, "X-Forwarded-For", 15) == 0) {
        self->current_field_ = Field::FORWARD_IP;
    } else if (strncasecmp(at, "User-Agent", 10) == 0) {
        self->current_field_ = Field::USER_AGENT;
    } else if (strncasecmp(at, "Content-Type", 12) == 0) {
        self->current_field_ = Field::CONTENT_TYPE;
    } else if (strncasecmp(at, "Content-Length", 14) == 0) {
        self->current_field_ = Field::CONTENT_LEN;
    } else if (strncasecmp(at, "Host", 4) == 0) {
        self->current_field_ = Field::HOST;
    } else {
        self->current_field_ = Field::NONE;
    }
    return 0;
}

int CHttpParserWrapper::OnHeaderValue(http_parser* parser, const char* at, size_t length, void* obj)
{
    auto self = static_cast<CHttpParserWrapper*>(obj);
    switch (self->current_field_) {
    case Field::REFERER:
        if (length > MAX_REFERER_LEN) length = MAX_REFERER_LEN;
        self->SetReferer(at, length);
        break;
    case Field::FORWARD_IP:
        self->SetForwardIP(at, length);
        break;
    case Field::USER_AGENT:
        self->SetUserAgent(at, length);
        break;
    case Field::CONTENT_TYPE:
        self->SetContentType(at, length);
        break;
    case Field::CONTENT_LEN: {
        std::string val(at, length);
        self->SetContentLength(atoi(val.c_str()));
        break;
    }
    case Field::HOST:
        self->SetHost(at, length);
        break;
    default:
        break;
    }
    self->current_field_ = Field::NONE;
    return 0;
}

int CHttpParserWrapper::OnHeadersComplete(http_parser* parser, void* obj)
{
    auto self = static_cast<CHttpParserWrapper*>(obj);
    self->SetMethod(parser->method);
    return 0;
}

int CHttpParserWrapper::OnBody(http_parser* parser, const char* at, size_t length, void* obj)
{
    auto self = static_cast<CHttpParserWrapper*>(obj);
    self->SetBody(at, length);
    return 0;
}

int CHttpParserWrapper::OnMessageComplete(http_parser* parser, void* obj)
{
    auto self = static_cast<CHttpParserWrapper*>(obj);
    self->SetCompleted();
    return 0;
}