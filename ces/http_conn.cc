
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string.h>

#include "http_conn.h"

#include "api_deal_sharefile.h"
#include "api_dealfile.h"
#include "api_login.h"
#include "api_md5.h"
#include "api_myfiles.h"
#include "api_register.h"
#include "api_sharefiles.h"
#include "api_sharepicture.h"
#include "api_upload.h"
#include "muduo/base/Logging.h"
#include "muduo/base/Timestamp.h"
#include "muduo/net/TcpConnection.h"

//////////////////////////
CHttpConn::CHttpConn(TcpConnectionPtr tcp_conn)
    : tcp_conn_(tcp_conn), http_state_(kHttpConnected) {
  busy_ = false;
  uuid_ = std::any_cast<uint32_t>(tcp_conn->getContext());
  init_time_ = Timestamp::getMicroseconds();
}

CHttpConn::~CHttpConn() {
  LOG_INFO << "uuid: " << uuid_
           << " http end: " << Timestamp::getMicroseconds() - init_time_
           << "us";
  // std::cout << "uuid: "<< uuid_ << " http end: " <<
  // Timestamp::getMicroseconds() - init_time_  << "us"<< std::endl;
  // std::cout.flush();
}

void CHttpConn::Close() {}

void CHttpConn::OnRead(Buffer *buf) // CHttpConn业务层面的OnRead
{
  const char *in_buf = buf->peek();
  size_t buf_len = buf->readableBytes();

  size_t nparsed = http_parser_.Parse(in_buf, buf_len);
  if (nparsed == 0) {
    // 解析出错，关闭连接
    LOG_ERROR << "HTTP parse error";
    // 关闭连接...
    return;
  }
  buf->retrieve(nparsed);
  // http_parser_.ParseHttpContent(in_buf, buf_len); // 1.
  // 从socket接口读取数据；2.然后把数据放到buffer in_buf; 3.http解析
  if (http_parser_.IsCompleted()) {
    std::string url = http_parser_.GetUrl();
    std::string content = http_parser_.GetBody();

    LOG_INFO << "url: " << url << ", content: " << content; // for debug

    if (strncmp(url.c_str(), "/api/reg", 8) ==
        0) { // 注册  url 路由。 根据根据url快速找到对应的处理函数，
             // 能不能使用map，hash
      _HandleRegisterRequest(url, content);
    } else if (strncmp(url.c_str(), "/api/login", 10) == 0) { // 登录
      _HandleLoginRequest(url, content);
    } else if (strncmp(url.c_str(), "/api/myfiles", 10) == 0) { //
      _HandleMyfilesRequest(url, content);
    } else if (strncmp(url.c_str(), "/api/sharefiles", 15) == 0) { //
      _HandleSharefilesRequest(url, content);
    } else if (strncmp(url.c_str(), "/api/dealfile", 13) == 0) { //
      _HandleDealfileRequest(url, content);
    } else if (strncmp(url.c_str(), "/api/dealsharefile", 18) == 0) { //
      _HandleDealsharefileRequest(url, content);
    } else if (strncmp(url.c_str(), "/api/sharepic", 13) == 0) { //
      _HandleSharepictureRequest(url, content);                  // 处理
    } else if (strncmp(url.c_str(), "/api/md5", 8) == 0) {       //
      _HandleMd5Request(url, content);                           // 处理
    } else if (strncmp(url.c_str(), "/api/upload", 11) == 0) {   // 上传
      _HandleUploadRequest(url, content);
    } else if (strncmp(url.c_str(), "/api/html", 9) == 0) { //  测试网页
      _HandleHtml(url, content);
    } else {
      LOG_ERROR << "url unknown, url= " << url;
      char *szContent = new char[HTTP_RESPONSE_JSON_MAX];
      string str_json = "{\"status\":\"bad request\"}";
      uint32_t ulen = str_json.size();
      snprintf(szContent, HTTP_RESPONSE_JSON_MAX, HTTP_RESPONSE_BAD_REQ, ulen,
               str_json.c_str());
      tcp_conn_->send(szContent);
    }
    http_parser_.Reset();
  }
}

void CHttpConn::OnWrite() {}

void CHttpConn::OnClose() { Close(); }

void CHttpConn::OnTimer(uint64_t curr_tick) {}
/*
OnRead, buf_len=1321, conn_handle=2, POST /api/upload HTTP/1.0
Host: 127.0.0.1:8081
Connection: close
Content-Length: 722
Accept: application/json, text/plain,
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML,
like Gecko) Chrome/106.0.0.0 Safari/537.36 Edg/106.0.1370.52 Content-Type:
multipart/form-data; boundary=----WebKitFormBoundaryjWE3qXXORSg2hZiB Origin:
http://114.215.169.66 Referer: http://114.215.169.66/myFiles Accept-Encoding:
gzip, deflate Accept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6
Cookie: userName=qingfuliao; token=e4252ae6e49176d51a5e87b41b6b9312

------WebKitFormBoundaryjWE3qXXORSg2hZiB
Content-Disposition: form-data; name="file_name"

config.ini
------WebKitFormBoundaryjWE3qXXORSg2hZiB
Content-Disposition: form-data; name="file_content_type"

application/octet-stream
------WebKitFormBoundaryjWE3qXXORSg2hZiB
Content-Disposition: form-data; name="file_path"

/root/tmp/5/0034880075
------WebKitFormBoundaryjWE3qXXORSg2hZiB
Content-Disposition: form-data; name="file_md5"

10f06f4707e9d108e9a9838de0f8ee33
------WebKitFormBoundaryjWE3qXXORSg2hZiB
Content-Disposition: form-data; name="file_size"

20
------WebKitFormBoundaryjWE3qXXORSg2hZiB
Content-Disposition: form-data; name="user"

qingfuliao
------WebKitFormBoundaryjWE3qXXORSg2hZiB--
*/
//

int CHttpConn::_HandleUploadRequest(string &url, string &post_data) {

  string str_json;
  int ret = ApiUpload(url, post_data, str_json);
  char *szContent = new char[HTTP_RESPONSE_JSON_MAX];
  uint32_t ulen = str_json.length();
  snprintf(szContent, HTTP_RESPONSE_JSON_MAX, HTTP_RESPONSE_JSON, ulen,
           str_json.c_str());
  tcp_conn_->send(szContent);
  // tcp_conn_->send(szContent); // 返回值暂时不做处理
  delete[] szContent;
  return 0;

  return 0;
}

// 账号注册处理
int CHttpConn::_HandleRegisterRequest(string &url, string &post_data) {

  string resp_json;
  int ret = ApiRegisterUser(url, post_data, resp_json);
  char *http_body = new char[HTTP_RESPONSE_JSON_MAX];
  uint32_t ulen = resp_json.length();
  snprintf(http_body, HTTP_RESPONSE_JSON_MAX, HTTP_RESPONSE_JSON, ulen,
           resp_json.c_str());
  tcp_conn_->send(http_body);
  delete[] http_body;
  // LOG_INFO << "tcp_conn_->send  ";
  LOG_INFO << "uuid: " << uuid_
           << " http send: " << Timestamp::getMicroseconds() - init_time_;
  return 0;
}
// 账号登陆处理 /api/login
#if API_LOGIN_MUTIL_THREAD
int CHttpConn::_HandleLoginRequest(string &url, string &post_data) {
  g_thread_pool.Exec(ApiUserLogin, uuid_, url, post_data);
  return 0;
}
#else
int CHttpConn::_HandleLoginRequest(string &url, string &post_data) {
  string str_json;
  int ret = ApiUserLogin(url, post_data, str_json);
  char *szContent = new char[HTTP_RESPONSE_JSON_MAX];
  uint32_t ulen = str_json.length();
  snprintf(szContent, HTTP_RESPONSE_JSON_MAX, HTTP_RESPONSE_JSON, ulen,
           str_json.c_str());
  // tcp_conn_->send(szContent);
  tcp_conn_->send(szContent);
  delete[] szContent;

  return 0;
}
#endif

//
int CHttpConn::_HandleDealfileRequest(string &url, string &post_data) {
  string str_json;
  int ret = ApiDealfile(url, post_data, str_json);
  char *szContent = new char[HTTP_RESPONSE_JSON_MAX];
  uint32_t ulen = str_json.length();
  snprintf(szContent, HTTP_RESPONSE_JSON_MAX, HTTP_RESPONSE_JSON, ulen,
           str_json.c_str());
  tcp_conn_->send(szContent);
  delete[] szContent;
  return 0;
}
//
int CHttpConn::_HandleDealsharefileRequest(string &url, string &post_data) {
  string str_json;
  int ret = ApiDealsharefile(url, post_data, str_json);
  char *szContent = new char[HTTP_RESPONSE_JSON_MAX];
  uint32_t ulen = str_json.length();
  snprintf(szContent, HTTP_RESPONSE_JSON_MAX, HTTP_RESPONSE_JSON, ulen,
           str_json.c_str());
  tcp_conn_->send(szContent);
  delete[] szContent;
  return 0;
}
//
int CHttpConn::_HandleMd5Request(string &url, string &post_data) {
  string str_json;
  int ret = ApiMd5(url, post_data, str_json);
  char *szContent = new char[HTTP_RESPONSE_JSON_MAX]; // 注意buffer的长度
  uint32_t ulen = str_json.length();
  snprintf(szContent, HTTP_RESPONSE_JSON_MAX, HTTP_RESPONSE_JSON, ulen,
           str_json.c_str());
  tcp_conn_->send(szContent);
  delete[] szContent;
  return 0;
}
#if API_MYFILES_MUTIL_THREAD
//  多线程版本
int CHttpConn::_HandleMyfilesRequest(string &url, string &post_data) {
  g_thread_pool.Exec(ApiMyfiles, uuid_, url, post_data);
  // 这里不应该再有数据了，
  return 0;
}
#else
//  单线程版本
int CHttpConn::_HandleMyfilesRequest(string &url, string &post_data) {
  string str_json;
  int ret = ApiMyfiles(url, post_data, str_json);
  char *szContent = new char[HTTP_RESPONSE_JSON_MAX]; // 注意buffer的长度
  uint32_t ulen = str_json.length();
  snprintf(szContent, HTTP_RESPONSE_JSON_MAX, HTTP_RESPONSE_JSON, ulen,
           str_json.c_str());
  tcp_conn_->send(szContent);
  delete[] szContent;
  return 0;
}
#endif
//
int CHttpConn::_HandleSharefilesRequest(string &url, string &post_data) {
  string str_json;
  int ret = ApiSharefiles(url, post_data, str_json);
  char *szContent = new char[HTTP_RESPONSE_JSON_MAX];
  uint32_t ulen = str_json.length();
  snprintf(szContent, HTTP_RESPONSE_JSON_MAX, HTTP_RESPONSE_JSON, ulen,
           str_json.c_str());
  tcp_conn_->send(szContent);
  delete[] szContent;
  return 0;
}
//
int CHttpConn::_HandleSharepictureRequest(string &url, string &post_data) {
  string str_json;
  int ret = ApiSharepicture(url, post_data, str_json);
  char *szContent = new char[HTTP_RESPONSE_JSON_MAX];
  uint32_t ulen = str_json.length();
  snprintf(szContent, HTTP_RESPONSE_JSON_MAX, HTTP_RESPONSE_JSON, ulen,
           str_json.c_str());
  tcp_conn_->send(szContent);
  delete[] szContent;
  return 0;
}
int CHttpConn::_HandleHtml(string &url, string &post_data) {
  std::ifstream fileStream("index.html");
  if (!fileStream.is_open()) {
    std::cerr << "无法打开文件。" << std::endl;
  }
  std::stringstream buffer;
  buffer << fileStream.rdbuf();

  char *szContent = new char[HTTP_RESPONSE_JSON_MAX];
  uint32_t ulen = buffer.str().size();
  snprintf(szContent, HTTP_RESPONSE_JSON_MAX, HTTP_RESPONSE_HTML, ulen,
           buffer.str().c_str());

  tcp_conn_->send(szContent);
  delete[] szContent;
  return 0;
}

void CHttpConn::OnWriteComlete() {
  // 标记状态
  http_state_ = kHttpResponseFinish;
}