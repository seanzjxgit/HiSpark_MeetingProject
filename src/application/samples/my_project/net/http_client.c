#include "http_client.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <string.h>
#include <stdio.h>

/* =====================================================================
 * 简单的TCP HTTP POST
 * ===================================================================== */
bool http_checkin(const char *user_id,
                  const char *time_str,
                  checkin_result_t *result)
{
    if(!result) return false;

    /* 构造JSON body */
    char body[128];
    snprintf(body, sizeof(body),
             "{\"user_id\":\"%s\",\"time\":\"%s\"}",
             user_id, time_str);

    /* 构造HTTP请求 */
    char request[512];
    int body_len = (int)strlen(body);
    snprintf(request, sizeof(request),
             "POST /checkin HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             SERVER_IP, SERVER_PORT, body_len, body);

    /* 创建socket */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) return false;

    /* 设置超时3秒 */
    struct timeval tv = { .tv_sec = 3, .tv_usec = 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    /* 连接服务器 */
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(SERVER_PORT);
    addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if(connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        snprintf(result->message, sizeof(result->message), "连接服务器失败");
        result->success = false;
        return false;
    }

    /* 发送请求 */
    send(sock, request, strlen(request), 0);

    /* 接收响应 */
    char response[1024] = {0};
    int  total = 0;
    int  n;
    while((n = recv(sock, response + total,
                    sizeof(response) - total - 1, 0)) > 0) {
        total += n;
    }
    close(sock);

    if(total == 0) {
        result->success = false;
        snprintf(result->message, sizeof(result->message), "无响应");
        return false;
    }

    /* 找到HTTP body（\r\n\r\n之后）*/
    char *body_start = strstr(response, "\r\n\r\n");
    if(!body_start) {
        result->success = false;
        return false;
    }
    body_start += 4;

    /* 简单解析JSON（不引入cJSON，手动提取关键字段）*/
    result->success = (strstr(body_start, "\"success\":true") != NULL);

    /* 提取 user_name */
    char *p = strstr(body_start, "\"user_name\":\"");
    if(p) {
        p += 13;
        char *end = strchr(p, '"');
        if(end) {
            int len = (int)(end - p);
            if(len >= (int)sizeof(result->user_name))
                len = (int)sizeof(result->user_name) - 1;
            strncpy(result->user_name, p, len);
            result->user_name[len] = '\0';
        }
    }

    /* 提取 status */
    p = strstr(body_start, "\"status\":\"");
    if(p) {
        p += 10;
        char *end = strchr(p, '"');
        if(end) {
            int len = (int)(end - p);
            if(len >= (int)sizeof(result->status))
                len = (int)sizeof(result->status) - 1;
            strncpy(result->status, p, len);
            result->status[len] = '\0';
        }
    }

    /* 提取 message */
    p = strstr(body_start, "\"message\":\"");
    if(p) {
        p += 11;
        char *end = strchr(p, '"');
        if(end) {
            int len = (int)(end - p);
            if(len >= (int)sizeof(result->message))
                len = (int)sizeof(result->message) - 1;
            strncpy(result->message, p, len);
            result->message[len] = '\0';
        }
    }

    return result->success;
}