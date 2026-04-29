USE modbus_scada;

-- 报警信息记录表（支持报警确认）
DROP TABLE IF EXISTS `t_alarm`;
CREATE TABLE `t_alarm` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '主键ID',
    `device_id` INT UNSIGNED NOT NULL COMMENT '业务设备ID',
    `reg_addr` INT UNSIGNED NOT NULL COMMENT '触发报警的寄存器地址',
    `tag_name` VARCHAR(128) NOT NULL COMMENT '触发报警的数据点标签',
    `alarm_type` TINYINT UNSIGNED NOT NULL COMMENT '报警类型：1-下限报警，2-上限报警，3-离线报警',
    `alarm_level` TINYINT UNSIGNED NOT NULL DEFAULT 2 COMMENT '报警级别：1-紧急，2-重要，3-一般',
    `alarm_value` DOUBLE NOT NULL COMMENT '触发报警的物理量值',
    `alarm_msg` VARCHAR(512) NOT NULL COMMENT '报警信息',
    `start_time` DATETIME(3) NOT NULL COMMENT '报警开始时间',
    `end_time` DATETIME(3) NULL COMMENT '报警结束时间',
    `is_confirmed` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '确认标志：0-未确认，1-已确认',
    `confirmed_by` VARCHAR(64) NULL COMMENT '确认人',
    `confirm_time` DATETIME(3) NULL COMMENT '确认时间',
    `confirm_remark` VARCHAR(512) NULL COMMENT '确认备注',
    PRIMARY KEY (`id`),
    KEY `idx_device_start` (`device_id`, `start_time`),
    KEY `idx_is_confirmed` (`is_confirmed`),
    KEY `idx_alarm_level` (`alarm_level`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Modbus TCP报警信息记录表'; 
