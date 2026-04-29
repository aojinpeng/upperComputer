# 导入依赖
from flask import Flask, request, jsonify
from flask_cors import CORS

# 创建Flask应用
app = Flask(__name__)
# 解决跨域问题，允许所有IP访问
CORS(app, supports_credentials=True)
# 核心接口：和你Qt代码里的uploadDataPoint完全匹配
# 路径：/upload，请求方式：POST
@app.route('/upload', methods=['POST'])
def receive_device_data():
    # 1. 解析你Qt发来的JSON数据
    data = request.get_json()
    # 2. 判空：如果没有JSON数据，返回错误
    if not data:
        return jsonify({"code": 400, "msg": "无效的JSON数据"}), 400
    
    # 3. 提取你Qt发来的字段（和你Qt里的JSON完全对应）
    device_id = data.get("device_id")
    tag_name = data.get("tag_name")
    value = data.get("value")
    timestamp = data.get("timestamp")
    is_alarm = data.get("is_alarm")
    alarm_msg = data.get("alarm_msg")

    # 4. 打印日志，让你看到收到的数据
    print("="*50)
    print(f"收到设备数据：")
    print(f"设备ID：{device_id}")
    print(f"标签名：{tag_name}")
    print(f"数值：{value}")
    print(f"时间：{timestamp}")
    print(f"是否报警：{is_alarm}")
    print(f"报警信息：{alarm_msg}")
    print("="*50)

    # 5. 返回响应给你的Qt程序，和你onUploadFinished适配
    return jsonify({
        "code": 200,
        "msg": "上传成功",
        "data": {"device_id": device_id}
    }), 200

# 程序入口
if __name__ == '__main__':
    # 监听0.0.0.0（所有IP可访问），端口8080，和你Qt里的端口匹配
    app.run(host="0.0.0.0", port=8080, debug=True)