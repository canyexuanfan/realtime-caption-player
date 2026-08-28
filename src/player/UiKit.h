// src/player/UiKit.h
// UI 复刻辅助件（对照 05_Single_HTML_Frontend_Reference.html）：
// - svgIcon：渲染 qrc 内 sprite 图标（描边风格，currentColor 换色）
// - Switch：滑块开关（.switch/.switch-track 复刻）
// - CaptionLabel：字幕描边/阴影文字绘制（.caption-final/.caption-partial 复刻）
// - Waveform：播放识别波形条（.waveform span 复刻，识别活动驱动）
#pragma once

#include <QAbstractButton>
#include <QApplication>
#include <QColor>
#include <QDateTime>
#include <QFile>
#include <QFontMetrics>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QSvgRenderer>
#include <QTimer>
#include <QVector>
#include <cmath>

namespace rcpui {

// 参考 sprite 为 24x24 描边图标：fill:none stroke:currentColor sw1.7 round。
inline QIcon svgIcon(const QString& name, const QColor& color, int px) {
    QFile f(QStringLiteral(":/icons/%1.svg").arg(name));
    if (!f.open(QIODevice::ReadOnly)) return QIcon();
    QString svg = QString::fromUtf8(f.readAll());
    // 注意：必须用 6 位 #RRGGBB——QSvgHandler 不认 Qt 私有的 8 位 #AARRGGBB，
    // 无效描边色会回落黑色，深色背景下图标"隐形"（快照实证）。
    svg.replace(QStringLiteral("currentColor"), color.name());
    const qreal dpr = qApp->devicePixelRatio();
    QSvgRenderer renderer(svg.toUtf8());
    QPixmap pm(static_cast<int>(px * dpr), static_cast<int>(px * dpr));
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    renderer.render(&p, QRectF(0, 0, px * dpr, px * dpr));
    p.end();
    pm.setDevicePixelRatio(dpr);
    return QIcon(pm);
}

// .switch/.switch-track 复刻：35x20 轨道 + 14px 圆钮，选中紫色渐变。
class Switch : public QAbstractButton {
public:
    explicit Switch(QWidget* parent = nullptr) : QAbstractButton(parent) {
        setCheckable(true);
        setCursor(Qt::PointingHandCursor);
        setFixedSize(35, 20);
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QRectF track(0.5, 0.5, 34, 19);
        if (isChecked()) {
            QLinearGradient g(track.topLeft(), track.bottomRight());
            g.setColorAt(0, QColor("#6756e6"));
            g.setColorAt(1, QColor("#7e6dff"));
            p.setPen(Qt::NoPen);
            p.setBrush(g);
        } else {
            p.setPen(QPen(QColor(255, 255, 255, 51), 1));
            p.setBrush(QColor("#4b515b"));
        }
        p.drawRoundedRect(track, 10, 10);
        const qreal kx = isChecked() ? 2 + 15 : 2;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#e9ebf0"));
        p.drawEllipse(QPointF(kx + 7, 10), 7, 7);
    }
};

// .caption-final（白字四向描边+投影）/ .caption-partial（灰字投影）复刻。
class CaptionLabel : public QLabel {
public:
    enum Style { Final, Partial };
    explicit CaptionLabel(QWidget* parent = nullptr) : QLabel(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        setWordWrap(true);
        setStyleSheet(QStringLiteral("background:transparent;"));
    }
    void setCaptionStyle(Style s) { m_style = s; update(); }
    void setCaptionColor(const QColor& c) { m_color = c; update(); }
    void setOutlineMode(int m) { m_outline = m; update(); }  // 0描边 1阴影 2关闭
    void setFontSizePx(int px) {
        QFont f = font();
        f.setPixelSize(px);
        f.setWeight(Final == m_style ? QFont::DemiBold : QFont::Normal);
        setFont(f);
        update();
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::TextAntialiasing);
        const QString t = text();
        if (t.isEmpty()) return;
        QFont f = font();
        const qreal wrapW = width() - 8;
        QTextOption opt;
        opt.setWrapMode(QTextOption::WordWrap);
        opt.setAlignment(Qt::AlignHCenter);
        // 简单换行：按像素宽度手工断行
        QStringList lines;
        {
            QString cur;
            for (const QChar ch : t) {
                if (FontMetricsF(f).horizontalAdvance(cur + ch) > wrapW && !cur.isEmpty()) {
                    lines << cur;
                    cur = ch;
                } else {
                    cur += ch;
                }
            }
            lines << cur;
        }
        const QFontMetrics fm(f);
        const qreal lineH = fm.height() * (Final == m_style ? 1.24 : 1.2);
        qreal y = (height() - lines.size() * lineH) / 2.0;
        if (y < 0) y = 0;
        for (const QString& line : lines) {
            const qreal x = (width() - fm.horizontalAdvance(line)) / 2.0;
            QPainterPath path;
            path.addText(QPointF(x, y + fm.ascent()), f, line);
            if (Partial == m_style) {
                QColor c = m_color;
                p.setBrush(c);
                // 投影
                QPainterPath shadow = path;
                shadow.translate(0, 2);
                p.setBrush(QColor(0, 0, 0, 230));
                p.drawPath(shadow);
                p.setBrush(c);
                p.drawPath(path);
            } else {
                if (2 != m_outline) {  // 描边或阴影
                    QPainterPath outline = path;
                    if (1 == m_outline) outline.translate(0, 4);
                    QPen pen(QColor(0, 0, 0, 240), 2 != m_outline ? 3 : 5);
                    pen.setJoinStyle(Qt::RoundJoin);
                    p.setPen(pen);
                    p.setBrush(Qt::NoBrush);
                    p.drawPath(outline);
                }
                p.setPen(Qt::NoPen);
                p.setBrush(m_color);
                p.drawPath(path);
            }
            y += lineH;
        }
    }
private:
    class FontMetricsF {
    public:
        explicit FontMetricsF(const QFont& f) : m_fm(f) {}
        qreal horizontalAdvance(const QString& s) const { return m_fm.horizontalAdvance(s); }
    private:
        QFontMetrics m_fm;
    };
    Style m_style = Final;
    QColor m_color = QColor("#ffffff");
    int m_outline = 0;
};

// .waveform 复刻：36 根 2px 圆角条，渐变 #a79cff→#6555ef，识别活动驱动。
class Waveform : public QWidget {
public:
    explicit Waveform(QWidget* parent = nullptr) : QWidget(parent) {
        setFixedHeight(14);
        setAttribute(Qt::WA_TransparentForMouseEvents);
        m_bars.fill(0.35, kBars);
        auto* t = new QTimer(this);
        connect(t, &QTimer::timeout, this, [this, t] {
            if (m_level > 0.02) {
                m_level *= 0.92;
                update();
            }
        });
        t->start(70);
    }
    void pulse(qreal level = 1.0) { m_level = qMax(m_level, level); update(); }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const int n = kBars;
        const int bw = 2, gap = 2;
        const int total = n * bw + (n - 1) * gap;
        int x = (width() - total) / 2;
        const qint64 t = QDateTime::currentMSecsSinceEpoch();
        for (int i = 0; i < n; ++i) {
            const qreal phase = std::sin((t / 260.0) + i * 0.55) * 0.5 + 0.5;
            qreal h = 3 + (m_bars[i % m_bars.size()]) * 9 * (0.35 + 0.65 * phase) * (0.25 + 0.75 * m_level);
            h = qBound<qreal>(2.0, h, 14.0);
            QLinearGradient g(x, 0, x, h);
            g.setColorAt(0, QColor("#a79cff"));
            g.setColorAt(1, QColor("#6555ef"));
            p.setPen(Qt::NoPen);
            p.setBrush(g);
            p.drawRoundedRect(QRectF(x, (height() - h) / 2.0, bw, h), 2, 2);
            x += bw + gap;
        }
    }
private:
    static constexpr int kBars = 36;
    QVector<qreal> m_bars{0.9, 0.5, 1.0, 0.6, 0.35, 0.8, 0.45, 0.95, 0.55, 0.75,
                          0.4, 0.85, 0.6, 1.0, 0.5, 0.7, 0.35, 0.9, 0.65, 0.45,
                          0.8, 0.55, 0.95, 0.4, 0.7, 0.6, 0.85, 0.5, 0.75, 0.35,
                          0.9, 0.65, 0.45, 0.8, 0.55, 0.7};
    qreal m_level = 0.0;
};

} // namespace rcpui
