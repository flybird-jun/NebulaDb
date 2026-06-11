# 迭代一
迭代一只实现基本的SQL语句执行能力，只支持基本的CREATE、DROP、INSERT、DELETE、UPDATE语句，不支持嵌套和子查询，也不支持表连接能力
- Compiler
1. 只支持基本的语句，CREATE、DROP、INSERT、DELETE、UPDATE, 不支持约束条件
2. 具备语法树转换成逻辑计划的能力
- Query
1. 具备逻辑计划转换成物理计划的能力
- Executor
1. 具备 CREATE、DROP等DDL语句执行能力
2. 具备DML语句物理计划执行能力
- DataModel
1. 具备管理表schma能力
- Sotrage
1. 具备页管理能力
2. 具备表数据管理能力
3. 具备建立索引以及索引查找能力
    - BTREE只支持key不超过256字节
4. 具备事务回滚、提交能力
- Common
1. 支持日志记录 --- 已完成
2. 支持内存管理 --- 已完成
3. 支持自旋锁和读写自旋锁 --- 已完成
- test
构建覆盖率测试框架，ASAN内存泄漏框架检测

迭代二
- 支持SQL表约束、触发器、嵌入子查询能力
- 支持逻辑计划基于启发式优化

迭代三
- 支持持久化
- Page Pool支持hash bucket以提升并发性能，实现LRU-K算法
- BTREE key长度支持不超过一个页，支持非唯一索引。
- 内存分配管理优化
- 自旋锁和读写锁优化
迭代四
- 逻辑计划基于代价的优化