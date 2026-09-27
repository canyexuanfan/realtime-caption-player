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
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QSvgRenderer>
#include <QTimer>
#include <QVector>
#include <QtMath>
#include <cmath>
#include <vector>

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
// C1：Tab 可聚焦 + hover 反馈 + 紫色 focus ring（审查 §2.4）。
class Switch : public QAbstractButton {
public:
    explicit Switch(QWidget* parent = nullptr) : QAbstractButton(parent) {
        setCheckable(true);
        setCursor(Qt::PointingHandCursor);
        setFixedSize(35, 20);
        setFocusPolicy(Qt::TabFocus);
        setAttribute(Qt::WA_Hover, true);
    }
protected:
    void enterEvent(QEnterEvent* e) override {
        m_hover = true; update(); QAbstractButton::enterEvent(e);
    }
    void leaveEvent(QEvent* e) override {
        m_hover = false; update(); QAbstractButton::leaveEvent(e);
    }
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
            p.setPen(QPen(m_hover ? QColor(255, 255, 255, 102) : QColor(255, 255, 255, 51), 1));
            p.setBrush(m_hover ? QColor("#575e6a") : QColor("#4b515b"));
        }
        p.drawRoundedRect(track, 10, 10);
        const qreal kx = isChecked() ? 2 + 15 : 2;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#e9ebf0"));
        p.drawEllipse(QPointF(kx + 7, 10), 7, 7);
        if (hasFocus()) {   // Tab 聚焦：紫色 focus ring
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(QColor("#7868ff"), 2));
            p.drawRoundedRect(track.adjusted(-3, -3, 3, 3), 12, 12);
        }
    }
    bool m_hover = false;
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
        // 参考 HTML font-family: Inter, "Segoe UI", "Microsoft YaHei UI", ...
        // 中文回退微软雅黑 UI；letter-spacing .02em(final)/.01em(partial)
        f.setFamilies({QStringLiteral("Segoe UI"), QStringLiteral("Microsoft YaHei UI"),
                       QStringLiteral("Microsoft YaHei")});
        f.setLetterSpacing(QFont::AbsoluteSpacing, px * (Final == m_style ? 0.02 : 0.01));
        setFont(f);
        // 全局 QSS `QWidget{font-size:14px}` 会覆盖 setFont 的字号（样式表字号优先于
        // 程序 setFont），导致字幕被压成 14px。必须在控件自身 stylesheet 里显式声明
        // font-size，自身规则优先级高于应用级规则；其余字体属性（族/字重/字距）仍由
        // setFont 提供，Qt 只覆盖样式表指明的 font-size 一项。
        setStyleSheet(QStringLiteral("background:transparent; font-size:%1px;").arg(px));
        update();
    }
protected:
    // 两趟盒式模糊近似高斯（CSS text-shadow blur）：只作用于 alpha（纯黑影子）。
    static void boxBlurAlpha(QImage& img, int r) {
        if (r <= 0 || img.isNull()) return;
        const int w = img.width(), h = img.height();
        QImage tmp(img.size(), img.format());
        std::vector<quint8> acc;
        // 水平
        for (int y = 0; y < h; ++y) {
            const QRgb* src = reinterpret_cast<const QRgb*>(img.constScanLine(y));
            QRgb* dst = reinterpret_cast<QRgb*>(tmp.scanLine(y));
            for (int x = 0; x < w; ++x) {
                int sum = 0, n = 0;
                for (int k = -r; k <= r; ++k) {
                    const int xx = x + k;
                    if (xx >= 0 && xx < w) { sum += qAlpha(src[xx]); ++n; }
                }
                dst[x] = qRgba(0, 0, 0, static_cast<int>(sum / n));
            }
        }
        // 垂直
        for (int x = 0; x < w; ++x) {
            for (int y = 0; y < h; ++y) {
                int sum = 0, n = 0;
                for (int k = -r; k <= r; ++k) {
                    const int yy = y + k;
                    if (yy >= 0 && yy < h) {
                        sum += qAlpha(reinterpret_cast<const QRgb*>(tmp.constScanLine(yy))[x]);
                        ++n;
                    }
                }
                reinterpret_cast<QRgb*>(img.scanLine(y))[x] =
                    qRgba(0, 0, 0, static_cast<int>(sum / n));
            }
        }
        Q_UNUSED(acc);
    }
    // 把文字路径渲染成纯黑模糊阴影图（pad 防裁边）；maxAlpha 为 CSS rgba 的
    // 不透明度（255 制）——影子中心不得满格，否则比设计浓、出现"磨砂玻璃"观感。
    static QImage makeBlurredShadow(const QPainterPath& path, const QRectF& bb,
                                    int radius, int maxAlpha = 255) {
        const int pad = radius * 2 + 4;
        QImage img(qCeil(bb.width()) + 2 * pad, qCeil(bb.height()) + 2 * pad,
                   QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::transparent);
        QPainter ip(&img);
        ip.setRenderHint(QPainter::Antialiasing);
        ip.setRenderHint(QPainter::TextAntialiasing);
        ip.translate(pad - bb.left(), pad - bb.top());
        ip.setPen(Qt::NoPen);
        ip.setBrush(Qt::black);
        ip.drawPath(path);
        ip.end();
        boxBlurAlpha(img, radius);
        boxBlurAlpha(img, radius);   // 两趟更接近高斯
        if (maxAlpha < 255) {
            for (int y = 0; y < img.height(); ++y) {
                QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
                for (int x = 0; x < img.width(); ++x) {
                    const int a = qAlpha(line[x]);
                    if (a > 0) line[x] = qRgba(0, 0, 0, a * maxAlpha / 255);
                }
            }
        }
        return img;
    }
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::TextAntialiasing);
        const QString t = text();
        if (t.isEmpty()) return;
        QFont f = font();
        const qreal wrapW = width() - 8;
        // 按像素宽度手工断行，整体垂直居中（CSS line-height 1.24/1.2）
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
        QPainterPath allPath;
        for (const QString& line : lines) {
            const qreal x = (width() - fm.horizontalAdvance(line)) / 2.0;
            allPath.addText(QPointF(x, y + fm.ascent()), f, line);
            y += lineH;
        }
        const QRectF bb = allPath.boundingRect();
        // 参考 CSS 阴影堆栈（逐层复刻，含各自 alpha——影子浓度不得高于设计）：
        // partial: 0 2px 3px rgba(0,0,0,.95)（紧贴层） + 0 0 8px rgba(0,0,0,.9)（扩散晕）
        // final:   四向 ±2px 实体 rgba(0,0,0,.94) + 0 4px 13px rgba(0,0,0,.85)
        const QString key = QStringLiteral("%1|%2|%3|%4").arg(t, QString::number(width()),
                            QString::number(int(m_style)), f.key());
        if (key != m_shadowKey) {
            m_shadowKey = key;
            if (Partial == m_style) {
                m_shadowImg = makeBlurredShadow(allPath, bb, 2, 242);    // 3px 层 .95
                m_shadowImg2 = makeBlurredShadow(allPath, bb, 4, 230);   // 8px 层 .90
            } else {
                m_shadowImg = makeBlurredShadow(allPath, bb, 9, 217);    // 13px 层 .85
                m_shadowImg2 = QImage();
            }
        }
        auto drawShadow = [&](qreal dx, qreal dy, const QImage& img) {
            if (img.isNull()) return;
            p.drawImage(QPointF(bb.left() - img.width() / 2.0 + dx,
                                bb.top() - img.height() / 2.0 + dy), img);
        };
        if (Partial == m_style) {
            drawShadow(0, 0, m_shadowImg2);   // 0 0 8px .90（扩散晕，无偏移）
            drawShadow(0, 2, m_shadowImg);    // 0 2px 3px .95（紧贴层后画，叠于晕上）
            p.setPen(Qt::NoPen);
            p.setBrush(m_color);
            p.drawPath(allPath);
        } else {
            if (2 != m_outline) {
                if (0 == m_outline) {
                    // 描边：四对角 2px 实体（CSS -2/2px 0 .94）
                    static const QPointF diag[] = {{-2, -2}, {2, -2}, {-2, 2}, {2, 2}};
                    p.setPen(Qt::NoPen);
                    p.setBrush(QColor(0, 0, 0, 240));
                    for (const QPointF& o : diag) {
                        QPainterPath t2 = allPath;
                        t2.translate(o);
                        p.drawPath(t2);
                    }
                }
                drawShadow(0, 4, m_shadowImg);   // 0 4px 13px .85
            }
            p.setPen(Qt::NoPen);
            p.setBrush(m_color);
            p.drawPath(allPath);
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
    QString m_shadowKey;    // 阴影图缓存键（文本+宽+样式+字体）
    QImage m_shadowImg;     // 主影子层
    QImage m_shadowImg2;    // partial 第二层（8px 晕）
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
                m_level *= 0.85;   // 快速衰减：说话间隙条回落，保持离散条形不连片
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
            qreal h = 3 + (m_bars[i % m_bars.size()]) * 7 * (0.35 + 0.65 * phase) * (0.25 + 0.75 * m_level);
            h = qBound<qreal>(2.0, h, 11.0);
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
