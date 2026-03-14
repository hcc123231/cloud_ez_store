#include "api_register.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api_common.h"
 #include <sys/time.h>
#include <time.h>

//解析用户注册信息的json包
/*json数据如下
    {
        userName:xxxx,
        nickName:xxx,
        firstPwd:xxx,
        phone:xxx,
        email:xxx
    }
    */
int decodeRegisterJson(const std::string &str_json, string &user_name,
                       string &nick_name, string &pwd, string &phone,
                       string &email) {
    bool res;
    Json::Value root;
    Json::Reader jsonReader;
    res = jsonReader.parse(str_json, root);
    if (!res) {
        LOG_ERROR << "parse reg json failed ";
        return -1;
    }

    // 用户名
    if (root["userName"].isNull()) {
        LOG_ERROR << "userName null";
        return -1;
    }
    user_name = root["userName"].asString();

    // 昵称
    if (root["nickName"].isNull()) {
        LOG_ERROR << "nickName null";
        return -1;
    }
    nick_name = root["nickName"].asString();

    //密码
    if (root["firstPwd"].isNull()) {
        LOG_ERROR << "firstPwd null";
        return -1;
    }
    pwd = root["firstPwd"].asString();

    //电话  非必须
    if (root["phone"].isNull()) {
        LOG_WARN << "phone null";
    } else {
        phone = root["phone"].asString();
    }

    //邮箱 非必须
    if (root["email"].isNull()) {
        LOG_WARN << "email null";
    } else {
        email = root["email"].asString();
    }

    return 0;
}

// 封装注册用户的json
int encodeRegisterJson(int ret, string &str_json) {
    Json::Value root;
    root["code"] = ret;
    Json::FastWriter writer;
    str_json = writer.write(root);
    return 0;
}

template <typename... Args>
std::string formatString2(const std::string &format, Args... args) {
    auto size = std::snprintf(nullptr, 0, format.c_str(), args...) +
                1; // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(),
                       buf.get() + size - 1); // We don't want the '\0' inside
}
int registerUser(string &user_name, string &nick_name, string &pwd,
                 string &phone, string &email) {
    int ret = 0;
    uint32_t user_id;
    CDBManager *db_manager = CDBManager::getInstance();
    CDBConn *db_conn = db_manager->GetDBConn("tuchuang_slave");
    AUTO_REL_DBCONN(db_manager, db_conn);
    // 先查看用户是否存在
    string str_sql;
    // str_sql =  "select * from user_info where user_name=" +  \' + user_name +
    // "/"";
    str_sql = formatString2("select id, user_name from user_info where user_name='%s'",
                           user_name.c_str());
    CResultSet *result_set = db_conn->ExecuteQuery(str_sql.c_str());
    if (result_set && result_set->Next()) { // 检测是否存在用户记录
        // 存在在返回
        LOG_WARN << "id: " << result_set->GetInt("id") << ", user_name: " << result_set->GetString("user_name")<<  "  已经存在";
        delete result_set;
        ret = 2;
    } else { // 如果不存在则注册
        time_t now;
        char create_time[TIME_STRING_LEN];
        //获取当前时间
        now = time(NULL);
        strftime(create_time, TIME_STRING_LEN - 1, "%Y-%m-%d %H:%M:%S",
                 localtime(&now));
        str_sql = "insert into user_info "
                 "(`user_name`,`nick_name`,`password`,`phone`,`email`,`create_"
                 "time`) values(?,?,?,?,?,?)";
        LOG_INFO << "执行: " <<  str_sql;
        // 必须在释放连接前delete
        // CPrepareStatement对象，否则有可能多个线程操作mysql对象，会crash
        CPrepareStatement *stmt = new CPrepareStatement();
        if (stmt->Init(db_conn->GetMysql(), str_sql)) {
            uint32_t index = 0;
            string c_time = create_time;
            stmt->SetParam(index++, user_name);
            stmt->SetParam(index++, nick_name);
            stmt->SetParam(index++, pwd);
            stmt->SetParam(index++, phone);
            stmt->SetParam(index++, email);
            stmt->SetParam(index++, c_time);
            bool bRet = stmt->ExecuteUpdate();
            if (bRet) {
                ret = 0;
                user_id = db_conn->GetInsertId();
                LOG_INFO << "insert user_id: " <<  user_id;
            } else {
                LOG_ERROR << "insert user_info failed. " <<  str_sql;
                ret = 1;
            }
        }
        delete stmt;
    }

    return ret;
}


#define HTTP_RESPONSE_JSON                                                     \
    "HTTP/1.1 200 OK\r\n"                                                      \
    "Connection:close\r\n"                                                     \
    "Content-Length:%d\r\n"                                                    \
    "Content-Type:application/json;charset=utf-8\r\n\r\n%s"
int ApiRegisterUser(string &url, string &post_data, string &resp_json) {

    // 封装注册结果
    // encodeRegisterJson(0, resp_json);
    // return 0;
    int ret = 0;
    string user_name;
    string nick_name;
    string pwd;
    string phone;
    string email;

    LOG_INFO << "post_data: " <<  post_data;

    // 判断数据是否为空
    if (post_data.empty()) {
        LOG_ERROR << "decodeRegisterJson failed";
        // 封装注册结果
        encodeRegisterJson(1, resp_json);
        return -1;
    }
    // 解析json
    if (decodeRegisterJson(post_data, user_name, nick_name, pwd, phone, email) < 0) {
        LOG_ERROR << "decodeRegisterJson failed";
        // 封装注册结果
        encodeRegisterJson(1, resp_json);
        return -1;
    }

    // 注册账号
    ret = registerUser(user_name, nick_name, pwd, phone, email); //先不操作数据库看看性能
 
    // 封装注册结果
    ret = encodeRegisterJson(ret, resp_json);


    return ret;
}