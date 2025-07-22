
#pragma once

#ifndef __DTO_HPP__
#define __DTO_HPP__

#include "stdint.h"

#include "kxutils/cxxjson.hpp"


typedef enum {
    ReqLoginCode = 1,
    ReqWiFiCode = 2,
    ReqOtaCode = 3,
}req_code_t;


typedef enum {
    ReportPingCode = 0,
    ReportOtaProgressCode = 1,

    RespLoginSuccessCode = 201,
    RespWiFiSuccessCode = 202,
    RespOtaSuccessCode = 203,

    RespLoginFailCode = 401,
    RespWiFiFailCode = 411,
    RespOtaFailCode = 421,
}resp_code_t;


typedef struct
{
    mJSON req_code;
    mJSON user_id;
    mJSON mac_address;
    mJSON data;
    mJSON msg_id;
}req_data_t;


class DTO {
public:
    static void resp_recv_login_sucess(const char *msg_id);
    static void resp_recv_wifi_sucess(const char *msg_id);
    static void resp_ota_sucess(const char *msg_id);
    static void resp_ota_fail(const char *msg_id);

    static void ping();
    static void report_ota_progress(int progress);

    static void set_ota_batch_id(const char *batch_id);

private:
    static char *ota_batch_id;

protected:

};

#endif /* __DTO_HPP__ */
