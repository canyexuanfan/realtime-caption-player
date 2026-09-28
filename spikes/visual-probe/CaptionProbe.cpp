// spikes/visual-probe/CaptionProbe.cpp
// 字幕渲染取证探针：灰底渲染 final/partial 字幕（与主程序同 CaptionLabel 路径），
// 输出 PNG 供与浏览器 CSS 渲染逐像素对比（影子浓度/扩散宽度/字号字重）。
// 用法：caption_probe <out.png>

#include "UiKit.h"

#include <QApplication>
#include <QPainter>
#include <QWidget>
#include <QVBoxLayout>
#include <cstdio>

class ProbePanel : public QWidget {
public:
    explicit ProbePanel(QWidget* parent = nullptr) : QWidget(parent) {
        setFixedSize(760, 240);
        setStyleSheet(QStringLiteral("background:#7f7f7f;"));
        auto* lay = new QVBoxLayout(this);
        lay->setContentsMargins(0, 30, 0, 20);
        lay->setSpacing(8);
        m_partial = new rcpui::CaptionLabel(this);
        m_partial->setCaptionStyle(rcpui::CaptionLabel::Partial);
        m_partial->setCaptionColor(QColor(225, 228, 234, 171));   // rgba(225,228,234,.67)
        m_partial->setFontSizePx(29);                              // 36*0.82
        m_partial->setText(QStringLiteral("从高空俯瞰"));
        m_partial->setFixedSize(620, 60);
        m_final = new rcpui::CaptionLabel(this);
        m_final->setCaptionStyle(rcpui::CaptionLabel::Final);
        m_final->setCaptionColor(QColor("#ffffff"));
        m_final->setFontSizePx(36);
        m_final->setText(QStringLiteral("一幅壮丽的画卷徐徐展开"));
        m_final->setFixedSize(620, 70);
        lay->addWidget(m_partial, 0, Qt::AlignHCenter);
        lay->addWidget(m_final, 0, Qt::AlignHCenter);
    }
    rcpui::CaptionLabel* m_partial = nullptr;
    rcpui::CaptionLabel* m_final = nullptr;
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    const QString out = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                 : QStringLiteral("caption_probe.png");
    ProbePanel panel;
    panel.show();
    {
        const QFont f = panel.m_final->font();
        std::printf("[font] family=%s pixelSize=%d pointSize=%.1f weight=%d letterSpacing=%.2f\n",
                    f.family().toUtf8().constData(), f.pixelSize(), f.pointSizeF(),
                    f.weight(), f.letterSpacing());
        QFontMetrics fm(f);
        std::printf("[font] 'yi' width=%d, full text width=%d\n",
                    fm.horizontalAdvance(QStringLiteral("\u4e00")),
                    fm.horizontalAdvance(QStringLiteral("\u4e00\u5e45\u58ee\u4e3d\u7684\u753b\u5377\u5f90\u5f90\u5c55\u5f00")));
        std::printf("[dpr] devicePixelRatio=%.2f\n", panel.devicePixelRatioF());
    }
    const QPixmap pm = panel.grab();
    pm.save(out);
    std::printf("saved %s %dx%d\n", out.toUtf8().constData(), pm.width(), pm.height());
    return 0;
}
