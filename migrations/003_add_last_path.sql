-- 003: 历史重开所需的最后打开路径（仅存本机数据库）。
ALTER TABLE media ADD COLUMN last_path TEXT NOT NULL DEFAULT '';
