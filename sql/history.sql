USE modbus_scada;

-- 历史数据记录表（按时间分区，工业场景建议按天/周分区）
DROP TABLE IF EXISTS `t_history_data`;
CREATE TABLE `t_history_data` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '主键ID',
    `device_id` INT UNSIGNED NOT NULL COMMENT '业务设备ID',
    `reg_addr` INT UNSIGNED NOT NULL COMMENT '寄存器地址',
    `tag_name` VARCHAR(128) NOT NULL COMMENT '数据点标签',
    `raw_value` INT UNSIGNED NOT NULL COMMENT '原始寄存器值',
    `scaled_value` DOUBLE NOT NULL COMMENT '缩放后的物理量值',
    `unit` VARCHAR(32) NOT NULL DEFAULT '' COMMENT '物理量单位',
    `timestamp` DATETIME(3) NOT NULL COMMENT '采集时间戳（毫秒级）',
    PRIMARY KEY (`id`, `timestamp`), -- 分区键必须在主键中
    KEY `idx_device_time` (`device_id`, `timestamp`),
    KEY `idx_tag_time` (`tag_name`, `timestamp`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Modbus TCP历史数据记录表'
/*!50100 PARTITION BY RANGE (TO_DAYS(`timestamp`))
(
    PARTITION p202601 VALUES LESS THAN (TO_DAYS('2026-02-01')),
    PARTITION p202602 VALUES LESS THAN (TO_DAYS('2026-03-01')),
    PARTITION p202603 VALUES LESS THAN (TO_DAYS('2026-04-01')),
    PARTITION p202604 VALUES LESS THAN (TO_DAYS('2026-05-01')),
    PARTITION p_future VALUES LESS THAN MAXVALUE
) */; 
