-- 创建数据库（如果不存在）
CREATE DATABASE IF NOT EXISTS modbus_scada DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE modbus_scada;

-- 设备信息表（支持软删除）
DROP TABLE IF EXISTS `t_device`;
CREATE TABLE `t_device` (
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '主键ID',
    `device_id` INT UNSIGNED NOT NULL COMMENT '业务设备ID（与config.json一致）',
    `device_name` VARCHAR(128) NOT NULL COMMENT '设备名称',
    `ip` VARCHAR(45) NOT NULL COMMENT '设备IP（支持IPv6）',
    `port` INT UNSIGNED NOT NULL DEFAULT 502 COMMENT 'Modbus TCP端口',
    `slave_id` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'Modbus从站ID',
    `start_addr` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '起始寄存器地址',
    `reg_count` INT UNSIGNED NOT NULL DEFAULT 10 COMMENT '寄存器数量',
    `alarm_min` DOUBLE NOT NULL DEFAULT 0.0 COMMENT '报警下限',
    `alarm_max` DOUBLE NOT NULL DEFAULT 100.0 COMMENT '报警上限',
    `scale_factor` DOUBLE NOT NULL DEFAULT 0.1 COMMENT '物理量缩放系数',
    `offset` DOUBLE NOT NULL DEFAULT 0.0 COMMENT '物理量偏移量',
    `unit` VARCHAR(32) NOT NULL DEFAULT '' COMMENT '物理量单位',
    `is_deleted` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '软删除标志：0-未删除，1-已删除',
    `create_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    `update_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_device_id` (`device_id`, `is_deleted`),
    KEY `idx_ip_port` (`ip`, `port`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Modbus TCP设备信息表'; 
