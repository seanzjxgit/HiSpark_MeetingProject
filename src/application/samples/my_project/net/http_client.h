#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <stdint.h>
#include <stdbool.h>

/* ===== 服务器配置（改成你电脑的IP）===== */
#define SERVER_IP    "192.168.1.100"   /* ← 改成运行server.py的电脑IP */
#define SERVER_PORT  8080

typedef struct {
    bool    success;
    char    user_name[32];
    char    status[16];     /* "checkin" / "checkout" / "unknown" */
    char    message[64];
} checkin_result_t;

bool http_checkin(const char *user_id,
                  const char *time_str,
                  checkin_result_t *result);

#endif