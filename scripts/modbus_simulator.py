#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Modbus TCP设备模拟器 - 适配5台设备 + 错误模拟功能
用于验证Qt面板：在线/离线/连接中/错误 四种状态
"""

import time
import random
import threading
from pymodbus.server.sync import StartTcpServer
from pymodbus.device import ModbusDeviceIdentification
from pymodbus.datastore import ModbusSequentialDataBlock, ModbusSlaveContext, ModbusServerContext

# ===================== 【升级1】适配你的5台设备，完全匹配config.json =====================
DEVICE_CONFIGS = [
    {
        "device_id": 1,
        "device_name": "温度传感器_01",
        "ip": "0.0.0.0",
        "port": 5020,
        "slave_id": 1,
        "start_addr": 0,
        "reg_count": 10,
        "type": "temp",
        "min": -10.0,
        "max": 80.0
    },
    {
        "device_id": 2,
        "device_name": "压力传感器_01",
        "ip": "0.0.0.0",
        "port": 5021,
        "slave_id": 1,
        "start_addr": 0,
        "reg_count": 5,
        "type": "pressure",
        "min": 0.0,
        "max": 10.0
    },
    {
        "device_id": 3,
        "device_name": "液位传感器_01",
        "ip": "0.0.0.0",
        "port": 5022,
        "slave_id": 1,
        "start_addr": 0,
        "reg_count": 8,
        "type": "level",
        "min": 0.0,
        "max": 5.0
    },
    {
        "device_id": 4,
        "device_name": "温度传感器_02",
        "ip": "0.0.0.0",
        "port": 5023,
        "slave_id": 1,
        "start_addr": 0,
        "reg_count": 10,
        "type": "temp",
        "min": -10.0,
        "max": 80.0
    },
    {
        "device_id": 5,
        "device_name": "压力传感器_02",
        "ip": "0.0.0.0",
        "port": 5024,
        "slave_id": 1,
        "start_addr": 0,
        "reg_count": 5,
        "type": "pressure",
        "min": 0.0,
        "max": 10.0
    }
]

# ===================== 【升级2】错误模拟配置 =====================
ERROR_CONFIG = {
    "offline_prob": 0.1,       # 10% 概率随机离线
    "error_prob": 0.15,        # 15% 概率数据超阈值（设备错误）
    "recover_time": 5          # 故障5秒后自动恢复
}

# 全局数据存储
device_contexts = {}
device_values = {}
device_online = {}
device_error = {}  # 设备错误状态标记
last_error_time = {}  # 故障时间记录

def generate_simulated_value(device_config, current_value):
    """生成带噪声/超阈值的模拟数据（支持错误模拟）"""
    device_id = device_config["device_id"]
    
    # 【核心：错误触发】随机生成超阈值数据 = 设备故障
    if random.random() < ERROR_CONFIG["error_prob"] and not device_error.get(device_id, False):
        device_error[device_id] = True
        last_error_time[device_id] = time.time()
        # 直接生成超出min/max的错误数据
        if device_config["type"] == "temp":
            return random.choice([-20.0, 90.0])
        elif device_config["type"] == "pressure":
            return random.choice([-1.0, 15.0])
        elif device_config["type"] == "level":
            return random.choice([-1.0, 8.0])

    # 故障自动恢复
    if device_error.get(device_id, False):
        if time.time() - last_error_time[device_id] > ERROR_CONFIG["recover_time"]:
            device_error[device_id] = False

    # 正常数据波动
    if device_config["type"] == "temp":
        new_val = current_value + random.uniform(-0.5, 0.5)
    elif device_config["type"] in ["pressure", "level"]:
        new_val = current_value + random.uniform(-0.1, 0.1)
    else:
        new_val = current_value
    
    return max(device_config["min"], min(device_config["max"], new_val))

def update_device_data():
    """后台线程：定期更新设备数据 + 状态管理"""
    time.sleep(2)
    while True:
        for config in DEVICE_CONFIGS:
            device_id = config["device_id"]
            
            # 【随机离线】10%概率设备离线
            if random.random() < ERROR_CONFIG["offline_prob"]:
                device_online[device_id] = False
                time.sleep(ERROR_CONFIG["recover_time"])
                device_online[device_id] = True

            # 只更新在线设备
            if not device_online.get(device_id, True) or device_id not in device_contexts:
                continue

            # 更新寄存器数据
            for i in range(config["reg_count"]):
                key = f"{device_id}_{i}"
                current_val = device_values.get(key, (config["max"] - config["min"]) / 2)
                new_val = generate_simulated_value(config, current_val)
                device_values[key] = new_val

                # 转换为Modbus协议数据
                reg_val = int(new_val * 10)
                context = device_contexts[device_id]
                context[0].setValues(3, config["start_addr"] + i, [reg_val])

        time.sleep(0.5)

def create_device_context(config):
    """创建Modbus设备上下文"""
    store = ModbusSlaveContext(
        hr=ModbusSequentialDataBlock(config["start_addr"], [0] * config["reg_count"]),
        unit=config["slave_id"]
    )
    return ModbusServerContext(slaves=store, single=True)

def start_device_server(config):
    """启动Modbus TCP服务器"""
    context = create_device_context(config)
    device_contexts[config["device_id"]] = context
    device_online[config["device_id"]] = True
    device_error[config["device_id"]] = False

    identity = ModbusDeviceIdentification()
    identity.VendorName = 'SCADA_SIM'
    identity.ProductName = 'MODBUS_DEVICE'
    
    StartTcpServer(context, identity=identity, address=(config["ip"], config["port"]))

def print_device_status():
    """后台打印设备状态（方便调试）"""
    while True:
        print("\n" + "="*50)
        print(f"{'设备ID':<6}{'设备名称':<12}{'状态':<10}{'端口':<8}")
        print("-"*50)
        for config in DEVICE_CONFIGS:
            did = config["device_id"]
            name = config["device_name"]
            port = config["port"]
            online = device_online.get(did, True)
            error = device_error.get(did, False)
            
            if not online:
                status = "🔴 离线"
            elif error:
                status = "🟠 错误"
            else:
                status = "🟢 在线"
            
            print(f"{did:<6}{name:<12}{status:<10}{port:<8}")
        time.sleep(2)

def main():
    print("🚀 启动Modbus设备模拟器（5台设备 + 错误模拟）...")
    
    # 启动所有设备服务器
    for config in DEVICE_CONFIGS:
        t = threading.Thread(target=start_device_server, args=(config,), daemon=True)
        t.start()
        time.sleep(0.3)
    
    # 启动数据更新线程
    threading.Thread(target=update_device_data, daemon=True).start()
    # 启动状态打印线程
    threading.Thread(target=print_device_status, daemon=True).start()

    print("\n✅ 模拟器启动完成！")
    print("📶 状态说明：🟢在线  🔴离线  🟠错误(数据超阈值)")
    print("💡 故障会自动恢复，用于测试Qt面板状态切换\n")

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n🛑 模拟器已停止")

if __name__ == "__main__":
    main()