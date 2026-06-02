#pragma once

// 学习注释：会话列表筛选类型，用于区分默认会话、收藏会话和归档会话。
// 使用模块：ApplicationController 控制筛选状态，ChatHistoryStorage 生成对应 SQL 查询，MainWindow 显示筛选控件。
enum class SessionListFilter {
    Active,
    Favorite,
    Archived
};
