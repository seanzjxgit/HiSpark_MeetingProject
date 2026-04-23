#!/usr/bin/env python3
"""
Hi3863 NFC打卡服务器
运行方式: python server.py
默认端口: 8080
"""

from http.server import HTTPServer, BaseHTTPRequestHandler
import json
import sqlite3
import os
from datetime import datetime

DB_PATH = "checkin.db"

# =====================================================================
# 数据库初始化
# =====================================================================
def init_db():
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()

    # 用户表：存储已注册用户
    c.execute("""
        CREATE TABLE IF NOT EXISTS users (
            user_id     TEXT PRIMARY KEY,   -- 6位ID，如"000001"
            user_name   TEXT NOT NULL,      -- 用户姓名
            created_at  TEXT NOT NULL
        )
    """)

    # 打卡记录表
    c.execute("""
        CREATE TABLE IF NOT EXISTS checkin_records (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id     TEXT    NOT NULL,   -- 打卡用户ID
            checkin_time TEXT   NOT NULL,   -- 打卡时间 "HH:MM"
            checkin_date TEXT   NOT NULL,   -- 打卡日期 "YYYY-MM-DD"
            status      TEXT    NOT NULL,   -- "checkin"签到 "checkout"签退
            created_at  TEXT    NOT NULL
        )
    """)

    # 预置一些用户（对应开发板烧录的ID）
    sample_users = [
        ("000001", "张三"),
        ("000002", "李四"),
        ("000003", "王五"),
    ]
    for uid, name in sample_users:
        c.execute("""
            INSERT OR IGNORE INTO users (user_id, user_name, created_at)
            VALUES (?, ?, ?)
        """, (uid, name, datetime.now().isoformat()))

    conn.commit()
    conn.close()
    print("[DB] 数据库初始化完成")


# =====================================================================
# 打卡处理逻辑
# =====================================================================
def handle_checkin(user_id: str, time_str: str) -> dict:
    """
    处理打卡请求
    返回: {"success": bool, "user_name": str, "status": str, "message": str}
    """
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()

    # 1. 查找用户是否存在
    c.execute("SELECT user_name FROM users WHERE user_id = ?", (user_id,))
    row = c.fetchone()

    if not row:
        conn.close()
        return {
            "success": False,
            "user_id": user_id,
            "user_name": "",
            "status": "unknown",
            "message": f"用户 {user_id} 未注册"
        }

    user_name = row[0]
    today = datetime.now().strftime("%Y-%m-%d")
    now_full = datetime.now().isoformat()

    # 2. 查今天是否已签到
    c.execute("""
        SELECT status FROM checkin_records
        WHERE user_id = ? AND checkin_date = ?
        ORDER BY id DESC LIMIT 1
    """, (user_id, today))
    last = c.fetchone()

    if last is None:
        # 今天第一次：签到
        status = "checkin"
        message = f"{user_name} 签到成功！时间: {time_str}"
    elif last[0] == "checkin":
        # 已签到：签退
        status = "checkout"
        message = f"{user_name} 签退成功！时间: {time_str}"
    else:
        # 已签退
        status = "already_done"
        message = f"{user_name} 今日已完成打卡"
        conn.close()
        return {
            "success": True,
            "user_id": user_id,
            "user_name": user_name,
            "status": status,
            "message": message
        }

    # 3. 写入记录
    c.execute("""
        INSERT INTO checkin_records
        (user_id, checkin_time, checkin_date, status, created_at)
        VALUES (?, ?, ?, ?, ?)
    """, (user_id, time_str, today, status, now_full))
    conn.commit()
    conn.close()

    print(f"[CHECKIN] {now_full} | {user_id} | {user_name} | {status}")
    return {
        "success": True,
        "user_id": user_id,
        "user_name": user_name,
        "status": status,
        "message": message
    }


def query_records(user_id: str = None, date: str = None) -> dict:
    """查询打卡记录"""
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()

    sql = """
        SELECT r.user_id, u.user_name, r.checkin_time,
               r.checkin_date, r.status
        FROM checkin_records r
        LEFT JOIN users u ON r.user_id = u.user_id
        WHERE 1=1
    """
    params = []
    if user_id:
        sql += " AND r.user_id = ?"
        params.append(user_id)
    if date:
        sql += " AND r.checkin_date = ?"
        params.append(date)
    sql += " ORDER BY r.id DESC LIMIT 100"

    c.execute(sql, params)
    rows = c.fetchall()
    conn.close()

    records = [
        {
            "user_id":      r[0],
            "user_name":    r[1],
            "checkin_time": r[2],
            "checkin_date": r[3],
            "status":       r[4]
        }
        for r in rows
    ]
    return {"success": True, "records": records, "count": len(records)}


# =====================================================================
# HTTP 请求处理
# =====================================================================
class CheckinHandler(BaseHTTPRequestHandler):

    def log_message(self, format, *args):
        pass  # 关闭默认日志，用自定义的

    def send_json(self, code: int, data: dict):
        body = json.dumps(data, ensure_ascii=False).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        """查询接口：GET /query?user_id=000001&date=2026-04-23"""
        if self.path.startswith("/query"):
            from urllib.parse import urlparse, parse_qs
            parsed = urlparse(self.path)
            params = parse_qs(parsed.query)
            user_id = params.get("user_id", [None])[0]
            date    = params.get("date",    [None])[0]
            result  = query_records(user_id, date)
            self.send_json(200, result)
        elif self.path == "/health":
            self.send_json(200, {"status": "ok"})
        else:
            self.send_json(404, {"error": "not found"})

    def do_POST(self):
        """打卡接口：POST /checkin  Body: {"user_id":"000001","time":"10:30"}"""
        if self.path == "/checkin":
            length = int(self.headers.get("Content-Length", 0))
            body   = self.rfile.read(length)
            try:
                data    = json.loads(body.decode("utf-8"))
                user_id = str(data.get("user_id", "")).strip()
                time_str= str(data.get("time",    "")).strip()

                if not user_id or not time_str:
                    self.send_json(400, {"success": False,
                                         "message": "缺少user_id或time"})
                    return

                result = handle_checkin(user_id, time_str)
                self.send_json(200, result)

            except Exception as e:
                self.send_json(500, {"success": False, "message": str(e)})
        else:
            self.send_json(404, {"error": "not found"})


# =====================================================================
# 启动服务器
# =====================================================================
if __name__ == "__main__":
    HOST = "0.0.0.0"
    PORT = 8080

    init_db()
    server = HTTPServer((HOST, PORT), CheckinHandler)

    import socket
    local_ip = socket.gethostbyname(socket.gethostname())
    print(f"[SERVER] 打卡服务器启动")
    print(f"[SERVER] 本机IP: {local_ip}:{PORT}")
    print(f"[SERVER] 打卡接口: POST http://{local_ip}:{PORT}/checkin")
    print(f"[SERVER] 查询接口: GET  http://{local_ip}:{PORT}/query")
    print(f"[SERVER] 数据库路径: {os.path.abspath(DB_PATH)}")
    print(f"[SERVER] 按 Ctrl+C 停止")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[SERVER] 服务器已停止")