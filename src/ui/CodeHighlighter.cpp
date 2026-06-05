#include "ui/CodeHighlighter.h"

#include <QRegularExpression>

// ─── 颜色常量 ────────────────────────────────────────────────────────
static const QColor kKeywordBlue(0x05, 0x50, 0xAE);
static const QColor kPreprocessorPurple(0x82, 0x50, 0xDF);
static const QColor kCommentGray(0x6E, 0x77, 0x81);
static const QColor kStringOrange(0x0A, 0x30, 0x69);
static const QColor kNumberGreen(0x05, 0x50, 0xAE);
static const QColor kVariableRed(0xCF, 0x22, 0x2E);

// ─── 格式工厂 ────────────────────────────────────────────────────────
static QTextCharFormat makeFormat(const QColor &color, bool bold = false)
{
    QTextCharFormat fmt;
    fmt.setForeground(color);
    if (bold) {
        fmt.setFontWeight(QFont::Bold);
    }
    return fmt;
}

// ─── 构造 ────────────────────────────────────────────────────────────
CodeHighlighter::CodeHighlighter(const QString &language, QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    setupRules(language.toLower());
}

// ─── 关键词注册 ──────────────────────────────────────────────────────
void CodeHighlighter::addKeywords(const QStringList &keywords, const QTextCharFormat &format)
{
    for (const QString &kw : keywords) {
        HighlightRule rule;
        rule.pattern = QRegularExpression(
            QStringLiteral("\\b") + QRegularExpression::escape(kw) + QStringLiteral("\\b"));
        rule.format = format;
        m_rules.append(rule);
    }
}

void CodeHighlighter::addRule(const QRegularExpression &pattern, const QTextCharFormat &format)
{
    m_rules.append({pattern, format});
}

void CodeHighlighter::setMultilineComment(const QString &start, const QString &end,
                                          const QTextCharFormat &format)
{
    m_multiLineStart  = QRegularExpression(QRegularExpression::escape(start));
    m_multiLineEnd    = QRegularExpression(QRegularExpression::escape(end));
    m_multiLineFormat = format;
}

// ─── highlightBlock ──────────────────────────────────────────────────
void CodeHighlighter::highlightBlock(const QString &text)
{
    // 单行规则
    for (const HighlightRule &rule : m_rules) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), rule.format);
        }
    }

    // 多行注释
    if (!m_multiLineStart.pattern().isEmpty()) {
        setCurrentBlockState(0);

        int startIndex = 0;
        if (previousBlockState() != 1) {
            QRegularExpressionMatch startMatch = m_multiLineStart.match(text);
            if (startMatch.hasMatch()) {
                startIndex = startMatch.capturedStart();
            } else {
                return;
            }
        }

        // 查找结束标记
        QRegularExpressionMatch endMatch = m_multiLineEnd.match(text, startIndex);
        int commentLength = 0;
        if (!endMatch.hasMatch()) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endMatch.capturedStart() + endMatch.capturedLength() - startIndex;
        }
        setFormat(startIndex, commentLength, m_multiLineFormat);
    }
}

// ─── 语言规则初始化 ──────────────────────────────────────────────────
void CodeHighlighter::setupRules(const QString &language)
{
    const QTextCharFormat keywordFmt  = makeFormat(kKeywordBlue, true);
    const QTextCharFormat commentFmt  = makeFormat(kCommentGray);
    const QTextCharFormat stringFmt   = makeFormat(kStringOrange);
    const QTextCharFormat numberFmt   = makeFormat(kNumberGreen);
    const QTextCharFormat preprocFmt  = makeFormat(kPreprocessorPurple);
    const QTextCharFormat variableFmt = makeFormat(kVariableRed);

    // ── 通用数字规则 ──
    addRule(QRegularExpression(QStringLiteral("\\b\\d+(\\.\\d+)?\\b")), numberFmt);

    if (language == QStringLiteral("cpp")) {
        // 关键词
        addKeywords(QStringList{
            QStringLiteral("if"), QStringLiteral("else"), QStringLiteral("for"),
            QStringLiteral("while"), QStringLiteral("return"), QStringLiteral("class"),
            QStringLiteral("struct"), QStringLiteral("int"), QStringLiteral("void"),
            QStringLiteral("bool"), QStringLiteral("auto"), QStringLiteral("const"),
            QStringLiteral("static"), QStringLiteral("virtual"), QStringLiteral("override"),
            QStringLiteral("include"), QStringLiteral("namespace"), QStringLiteral("using"),
            QStringLiteral("template"), QStringLiteral("typename"), QStringLiteral("public"),
            QStringLiteral("private"), QStringLiteral("protected"), QStringLiteral("switch"),
            QStringLiteral("case"), QStringLiteral("break"), QStringLiteral("default"),
            QStringLiteral("do"), QStringLiteral("enum"), QStringLiteral("explicit"),
            QStringLiteral("extern"), QStringLiteral("float"), QStringLiteral("double"),
            QStringLiteral("char"), QStringLiteral("long"), QStringLiteral("short"),
            QStringLiteral("unsigned"), QStringLiteral("signed"), QStringLiteral("sizeof"),
            QStringLiteral("typedef"), QStringLiteral("union"), QStringLiteral("volatile"),
            QStringLiteral("try"), QStringLiteral("catch"), QStringLiteral("throw"),
            QStringLiteral("new"), QStringLiteral("delete"), QStringLiteral("this"),
            QStringLiteral("friend"), QStringLiteral("inline"), QStringLiteral("mutable"),
            QStringLiteral("operator"), QStringLiteral("register"),
        }, keywordFmt);

        // 预处理指令
        addRule(QRegularExpression(QStringLiteral("^\\s*#\\s*\\w+")), preprocFmt);

        // 行注释
        addRule(QRegularExpression(QStringLiteral("//[^\n]*")), commentFmt);

        // 字符串
        addRule(QRegularExpression(QStringLiteral("\"(?:[^\"\\\\]|\\\\.)*\"")), stringFmt);

        // 块注释（多行）
        setMultilineComment(QStringLiteral("/*"), QStringLiteral("*/"), commentFmt);

    } else if (language == QStringLiteral("python")) {
        addKeywords(QStringList{
            QStringLiteral("def"), QStringLiteral("class"), QStringLiteral("import"),
            QStringLiteral("from"), QStringLiteral("return"), QStringLiteral("if"),
            QStringLiteral("elif"), QStringLiteral("else"), QStringLiteral("for"),
            QStringLiteral("while"), QStringLiteral("try"), QStringLiteral("except"),
            QStringLiteral("finally"), QStringLiteral("with"), QStringLiteral("as"),
            QStringLiteral("True"), QStringLiteral("False"), QStringLiteral("None"),
            QStringLiteral("and"), QStringLiteral("or"), QStringLiteral("not"),
            QStringLiteral("in"), QStringLiteral("is"), QStringLiteral("lambda"),
            QStringLiteral("yield"), QStringLiteral("raise"), QStringLiteral("pass"),
            QStringLiteral("break"), QStringLiteral("continue"), QStringLiteral("self"),
            QStringLiteral("global"), QStringLiteral("nonlocal"), QStringLiteral("del"),
            QStringLiteral("assert"),
        }, keywordFmt);

        addRule(QRegularExpression(QStringLiteral("#[^\n]*")), commentFmt);
        addRule(QRegularExpression(QStringLiteral("\"(?:[^\"\\\\]|\\\\.)*\"")), stringFmt);
        addRule(QRegularExpression(QStringLiteral("'(?:[^'\\\\]|\\\\.)*'")), stringFmt);
        // 三引号字符串（多行）
        setMultilineComment(QStringLiteral("\"\"\""), QStringLiteral("\"\"\""), stringFmt);

    } else if (language == QStringLiteral("js") || language == QStringLiteral("javascript")) {
        addKeywords(QStringList{
            QStringLiteral("const"), QStringLiteral("let"), QStringLiteral("var"),
            QStringLiteral("function"), QStringLiteral("return"), QStringLiteral("if"),
            QStringLiteral("else"), QStringLiteral("for"), QStringLiteral("while"),
            QStringLiteral("class"), QStringLiteral("import"), QStringLiteral("export"),
            QStringLiteral("from"), QStringLiteral("default"), QStringLiteral("async"),
            QStringLiteral("await"), QStringLiteral("try"), QStringLiteral("catch"),
            QStringLiteral("throw"), QStringLiteral("new"), QStringLiteral("this"),
            QStringLiteral("typeof"), QStringLiteral("instanceof"), QStringLiteral("true"),
            QStringLiteral("false"), QStringLiteral("null"), QStringLiteral("undefined"),
            QStringLiteral("switch"), QStringLiteral("case"), QStringLiteral("break"),
            QStringLiteral("continue"), QStringLiteral("of"), QStringLiteral("extends"),
            QStringLiteral("super"), QStringLiteral("static"), QStringLiteral("get"),
            QStringLiteral("set"), QStringLiteral("yield"),
        }, keywordFmt);

        addRule(QRegularExpression(QStringLiteral("//[^\n]*")), commentFmt);
        addRule(QRegularExpression(QStringLiteral("\"(?:[^\"\\\\]|\\\\.)*\"")), stringFmt);
        addRule(QRegularExpression(QStringLiteral("'(?:[^'\\\\]|\\\\.)*'")), stringFmt);
        addRule(QRegularExpression(QStringLiteral("`(?:[^`\\\\]|\\\\.)*`")), stringFmt);
        setMultilineComment(QStringLiteral("/*"), QStringLiteral("*/"), commentFmt);

    } else if (language == QStringLiteral("bash") || language == QStringLiteral("sh")) {
        addKeywords(QStringList{
            QStringLiteral("if"), QStringLiteral("then"), QStringLiteral("else"),
            QStringLiteral("fi"), QStringLiteral("for"), QStringLiteral("while"),
            QStringLiteral("do"), QStringLiteral("done"), QStringLiteral("case"),
            QStringLiteral("esac"), QStringLiteral("function"), QStringLiteral("return"),
            QStringLiteral("local"), QStringLiteral("export"), QStringLiteral("echo"),
            QStringLiteral("exit"), QStringLiteral("source"), QStringLiteral("in"),
            QStringLiteral("read"), QStringLiteral("shift"), QStringLiteral("unset"),
            QStringLiteral("declare"), QStringLiteral("typeset"),
        }, keywordFmt);

        addRule(QRegularExpression(QStringLiteral("#[^\n]*")), commentFmt);
        addRule(QRegularExpression(QStringLiteral("\"(?:[^\"\\\\]|\\\\.)*\"")), stringFmt);
        // 变量引用
        addRule(QRegularExpression(QStringLiteral("\\$\\{?[A-Za-z_][A-Za-z0-9_]*\\}?")), variableFmt);

    } else if (language == QStringLiteral("sql")) {
        // SQL 关键词（大小写不敏感）
        QRegularExpression kwPattern(
            QStringLiteral(
                "\\b(?:SELECT|FROM|WHERE|INSERT|INTO|VALUES|UPDATE|SET|DELETE|CREATE|TABLE|"
                "ALTER|DROP|JOIN|LEFT|RIGHT|INNER|ON|AND|OR|NOT|NULL|AS|ORDER|BY|GROUP|"
                "HAVING|LIMIT|OFFSET|UNION|ALL|DISTINCT|COUNT|SUM|AVG|MAX|MIN|BETWEEN|"
                "LIKE|IN|EXISTS|CASE|WHEN|THEN|ELSE|END|INDEX|PRIMARY|KEY|FOREIGN|"
                "REFERENCES|ADD|COLUMN|DATABASE|VIEW|IF|IS|ASC|DESC|TOP|WITH)\\b"),
            QRegularExpression::CaseInsensitiveOption);
        addRule(kwPattern, keywordFmt);

        addRule(QRegularExpression(QStringLiteral("--[^\n]*")), commentFmt);
        addRule(QRegularExpression(QStringLiteral("'(?:[^'\\\\]|\\\\.)*'")), stringFmt);
        setMultilineComment(QStringLiteral("/*"), QStringLiteral("*/"), commentFmt);

    } else if (language == QStringLiteral("java")) {
        addKeywords(QStringList{
            QStringLiteral("public"), QStringLiteral("private"), QStringLiteral("protected"),
            QStringLiteral("class"), QStringLiteral("interface"), QStringLiteral("extends"),
            QStringLiteral("implements"), QStringLiteral("static"), QStringLiteral("final"),
            QStringLiteral("void"), QStringLiteral("int"), QStringLiteral("boolean"),
            QStringLiteral("String"), QStringLiteral("return"), QStringLiteral("if"),
            QStringLiteral("else"), QStringLiteral("for"), QStringLiteral("while"),
            QStringLiteral("do"), QStringLiteral("switch"), QStringLiteral("case"),
            QStringLiteral("break"), QStringLiteral("continue"), QStringLiteral("new"),
            QStringLiteral("this"), QStringLiteral("super"), QStringLiteral("try"),
            QStringLiteral("catch"), QStringLiteral("finally"), QStringLiteral("throw"),
            QStringLiteral("throws"), QStringLiteral("import"), QStringLiteral("package"),
            QStringLiteral("null"), QStringLiteral("true"), QStringLiteral("false"),
            QStringLiteral("abstract"), QStringLiteral("synchronized"), QStringLiteral("volatile"),
            QStringLiteral("transient"), QStringLiteral("native"), QStringLiteral("enum"),
        }, keywordFmt);

        addRule(QRegularExpression(QStringLiteral("//[^\n]*")), commentFmt);
        addRule(QRegularExpression(QStringLiteral("\"(?:[^\"\\\\]|\\\\.)*\"")), stringFmt);
        setMultilineComment(QStringLiteral("/*"), QStringLiteral("*/"), commentFmt);

    } else if (language == QStringLiteral("go")) {
        addKeywords(QStringList{
            QStringLiteral("func"), QStringLiteral("return"), QStringLiteral("if"),
            QStringLiteral("else"), QStringLiteral("for"), QStringLiteral("range"),
            QStringLiteral("switch"), QStringLiteral("case"), QStringLiteral("default"),
            QStringLiteral("break"), QStringLiteral("continue"), QStringLiteral("go"),
            QStringLiteral("defer"), QStringLiteral("select"), QStringLiteral("chan"),
            QStringLiteral("map"), QStringLiteral("struct"), QStringLiteral("interface"),
            QStringLiteral("type"), QStringLiteral("package"), QStringLiteral("import"),
            QStringLiteral("var"), QStringLiteral("const"), QStringLiteral("nil"),
            QStringLiteral("true"), QStringLiteral("false"), QStringLiteral("make"),
            QStringLiteral("new"), QStringLiteral("append"), QStringLiteral("len"),
            QStringLiteral("cap"), QStringLiteral("int"), QStringLiteral("string"),
            QStringLiteral("bool"), QStringLiteral("float64"), QStringLiteral("error"),
        }, keywordFmt);

        addRule(QRegularExpression(QStringLiteral("//[^\n]*")), commentFmt);
        addRule(QRegularExpression(QStringLiteral("\"(?:[^\"\\\\]|\\\\.)*\"")), stringFmt);
        addRule(QRegularExpression(QStringLiteral("`(?:[^`\\\\]|\\\\.)*`")), stringFmt);
        setMultilineComment(QStringLiteral("/*"), QStringLiteral("*/"), commentFmt);

    } else if (language == QStringLiteral("rust")) {
        addKeywords(QStringList{
            QStringLiteral("fn"), QStringLiteral("let"), QStringLiteral("mut"),
            QStringLiteral("const"), QStringLiteral("static"), QStringLiteral("struct"),
            QStringLiteral("enum"), QStringLiteral("trait"), QStringLiteral("impl"),
            QStringLiteral("pub"), QStringLiteral("use"), QStringLiteral("mod"),
            QStringLiteral("self"), QStringLiteral("Self"), QStringLiteral("super"),
            QStringLiteral("crate"), QStringLiteral("match"), QStringLiteral("if"),
            QStringLiteral("else"), QStringLiteral("loop"), QStringLiteral("while"),
            QStringLiteral("for"), QStringLiteral("in"), QStringLiteral("return"),
            QStringLiteral("break"), QStringLiteral("continue"), QStringLiteral("where"),
            QStringLiteral("as"), QStringLiteral("ref"), QStringLiteral("move"),
            QStringLiteral("unsafe"), QStringLiteral("extern"), QStringLiteral("type"),
            QStringLiteral("true"), QStringLiteral("false"), QStringLiteral("dyn"),
            QStringLiteral("async"), QStringLiteral("await"),
        }, keywordFmt);

        addRule(QRegularExpression(QStringLiteral("//[^\n]*")), commentFmt);
        addRule(QRegularExpression(QStringLiteral("\"(?:[^\"\\\\]|\\\\.)*\"")), stringFmt);
        addRule(QRegularExpression(QStringLiteral("r#\"(?:[^\"\\\\]|\\\\.)*\"#")), stringFmt);
        setMultilineComment(QStringLiteral("/*"), QStringLiteral("*/"), commentFmt);

    } else if (language == QStringLiteral("json")) {
        addRule(QRegularExpression(QStringLiteral("\"(?:[^\"\\\\]|\\\\.)*\"\\s*:")),
                makeFormat(kKeywordBlue, true));
        addRule(QRegularExpression(QStringLiteral("\"(?:[^\"\\\\]|\\\\.)*\"")), stringFmt);

    } else if (language == QStringLiteral("xml") || language == QStringLiteral("html")) {
        addRule(QRegularExpression(QStringLiteral("<!--[^>]*-->")), commentFmt);
        addRule(QRegularExpression(QStringLiteral("<[^>]+>")),
                makeFormat(kPreprocessorPurple));
        addRule(QRegularExpression(QStringLiteral("\"(?:[^\"\\\\]|\\\\.)*\"")), stringFmt);

    } else if (language == QStringLiteral("yaml") || language == QStringLiteral("yml")) {
        addRule(QRegularExpression(QStringLiteral("#[^\n]*")), commentFmt);
        addRule(QRegularExpression(QStringLiteral("^\\s*[A-Za-z_][A-Za-z0-9_-]*\\s*:")),
                makeFormat(kKeywordBlue, true));
        addRule(QRegularExpression(QStringLiteral("\"(?:[^\"\\\\]|\\\\.)*\"")), stringFmt);
        addRule(QRegularExpression(QStringLiteral("'(?:[^'\\\\]|\\\\.)*'")), stringFmt);

    } else if (language == QStringLiteral("cmake")) {
        addKeywords(QStringList{
            QStringLiteral("cmake_minimum_required"), QStringLiteral("project"),
            QStringLiteral("set"), QStringLiteral("find_package"), QStringLiteral("add_executable"),
            QStringLiteral("add_library"), QStringLiteral("target_link_libraries"),
            QStringLiteral("target_include_directories"), QStringLiteral("target_compile_options"),
            QStringLiteral("if"), QStringLiteral("else"), QStringLiteral("endif"),
            QStringLiteral("foreach"), QStringLiteral("endforeach"), QStringLiteral("function"),
            QStringLiteral("endfunction"), QStringLiteral("macro"), QStringLiteral("endmacro"),
            QStringLiteral("include"), QStringLiteral("option"), QStringLiteral("message"),
            QStringLiteral("file"), QStringLiteral("add_subdirectory"), QStringLiteral("enable_testing"),
            QStringLiteral("add_test"), QStringLiteral("qt_add_executable"), QStringLiteral("qt_add_resources"),
            QStringLiteral("qt_standard_project_setup"),
        }, keywordFmt);

        addRule(QRegularExpression(QStringLiteral("#[^\n]*")), commentFmt);
        addRule(QRegularExpression(QStringLiteral("\"(?:[^\"\\\\]|\\\\.)*\"")), stringFmt);
        addRule(QRegularExpression(QStringLiteral("\\$\\{[^}]+\\}")), variableFmt);

    } else {
        // 未知语言：只做字符串和数字的基础着色
        addRule(QRegularExpression(QStringLiteral("\"(?:[^\"\\\\]|\\\\.)*\"")), stringFmt);
        addRule(QRegularExpression(QStringLiteral("'(?:[^'\\\\]|\\\\.)*'")), stringFmt);
    }
}
