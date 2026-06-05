#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>

/// 功能：为不同编程语言提供语法着色；使用模块：MessageWidget::addCodeBlock。
class CodeHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    /// 功能：根据语言标识符构造高亮器并初始化对应规则。
    /// @param language 代码语言标识（cpp / python / js / bash / sql / java / go / rust / json / xml / yaml / cmake）
    /// @param parent   关联的 QTextDocument
    explicit CodeHighlighter(const QString &language, QTextDocument *parent = nullptr);

protected:
    /// 功能：逐行着色，由 QSyntaxHighlighter 框架调用。
    void highlightBlock(const QString &text) override;

private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat  format;
    };

    /// 功能：注册一组关键词。
    void addKeywords(const QStringList &keywords, const QTextCharFormat &format);
    /// 功能：注册单行规则（正则匹配 → 格式）。
    void addRule(const QRegularExpression &pattern, const QTextCharFormat &format);
    /// 功能：设置多行注释规则。
    void setMultilineComment(const QString &start, const QString &end, const QTextCharFormat &format);

    /// 功能：初始化指定语言的着色规则。
    void setupRules(const QString &language);

    QVector<HighlightRule> m_rules;
    QRegularExpression     m_multiLineStart;
    QRegularExpression     m_multiLineEnd;
    QTextCharFormat        m_multiLineFormat;
};
