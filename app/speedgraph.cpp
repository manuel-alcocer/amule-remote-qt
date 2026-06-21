#include "speedgraph.h"

#include <algorithm>

#include <QPainter>
#include <QPainterPath>

#include "model/model.h"

namespace amule {

namespace {
const QColor kDownColor(0x4c, 0xc0, 0x6a);
const QColor kUpColor(0x4a, 0x90, 0xd9);
} // namespace

SpeedGraph::SpeedGraph(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(70);
}

QSize SpeedGraph::sizeHint() const {
    return {400, 80};
}

void SpeedGraph::addSample(quint64 downBytesPerSec, quint64 upBytesPerSec) {
    down_.append(downBytesPerSec);
    up_.append(upBytesPerSec);
    while (down_.size() > kMaxSamples)
        down_.removeFirst();
    while (up_.size() > kMaxSamples)
        up_.removeFirst();
    update();
}

void SpeedGraph::clear() {
    down_.clear();
    up_.clear();
    update();
}

void SpeedGraph::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF area = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    painter.fillRect(area, palette().base());
    painter.setPen(palette().mid().color());
    painter.drawRect(area);

    const qsizetype n = down_.size();
    quint64 peak = 1;
    for (qsizetype i = 0; i < n; ++i)
        peak = std::max({peak, down_.at(i), up_.at(i)});

    const auto buildPath = [&](const QList<quint64>& series, bool fill) {
        QPainterPath path;
        if (series.isEmpty())
            return path;
        const double w = area.width();
        const double h = area.height();
        const auto xAt = [&](qsizetype i) {
            return area.left() + (n <= 1 ? w : w * double(i) / double(n - 1));
        };
        const auto yAt = [&](quint64 v) {
            return area.bottom() - (h * double(v) / double(peak));
        };
        if (fill)
            path.moveTo(xAt(0), area.bottom());
        else
            path.moveTo(xAt(0), yAt(series.at(0)));
        for (qsizetype i = 0; i < series.size(); ++i)
            path.lineTo(xAt(i), yAt(series.at(i)));
        if (fill)
            path.lineTo(xAt(series.size() - 1), area.bottom());
        return path;
    };

    // Download as a filled area; upload as a line on top.
    QColor fillColor = kDownColor;
    fillColor.setAlpha(70);
    painter.fillPath(buildPath(down_, true), fillColor);
    painter.setPen(QPen(kDownColor, 1.5));
    painter.drawPath(buildPath(down_, false));
    painter.setPen(QPen(kUpColor, 1.5));
    painter.drawPath(buildPath(up_, false));

    // Current-rate legend.
    const quint64 curDown = down_.isEmpty() ? 0 : down_.last();
    const quint64 curUp = up_.isEmpty() ? 0 : up_.last();
    painter.setPen(palette().text().color());
    painter.drawText(area.adjusted(6, 4, -6, -4), Qt::AlignTop | Qt::AlignLeft,
                     QStringLiteral("↓ %1   ↑ %2")
                         .arg(humanRate(curDown), humanRate(curUp)));
    painter.setPen(palette().mid().color());
    painter.drawText(area.adjusted(6, 4, -6, -4), Qt::AlignTop | Qt::AlignRight,
                     QStringLiteral("peak %1").arg(humanRate(peak)));
}

} // namespace amule
