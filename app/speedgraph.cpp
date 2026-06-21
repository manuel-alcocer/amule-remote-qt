#include "speedgraph.h"

#include <algorithm>

#include <cmath>

#include <QDataStream>
#include <QFile>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QSaveFile>
#include <QTimer>

#include "model/model.h"

namespace amule {

namespace {
constexpr quint32 kMagic = 0x53504731; // "SPG1"
} // namespace

SpeedGraph::SpeedGraph(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(70);

    dismissTimer_ = new QTimer(this);
    dismissTimer_->setInterval(50);
    connect(dismissTimer_, &QTimer::timeout, this, [this] {
        scrollCols_ += 2; // LEDs per tick
        update();
    });
}

void SpeedGraph::showMessage(const QString& text) {
    message_ = text;
    scrollCols_ = 0;
    mode_ = Mode::Message;
    if (dismissTimer_)
        dismissTimer_->stop();
    update();
}

void SpeedGraph::dismissMessage() {
    if (mode_ == Mode::Message && dismissTimer_)
        dismissTimer_->start();
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

void SpeedGraph::save(const QString& path) const {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return;
    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_5);
    out << kMagic << down_ << up_;
    file.commit();
}

void SpeedGraph::load(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_5);
    quint32 magic = 0;
    in >> magic;
    if (magic != kMagic)
        return;
    in >> down_ >> up_;
    while (down_.size() > kMaxSamples)
        down_.removeFirst();
    while (up_.size() > kMaxSamples)
        up_.removeFirst();
    update();
}

void SpeedGraph::paintEvent(QPaintEvent*) {
    QPainter painter(this);

    // LED dot-matrix panel: black background, a grid of dim dots, with the
    // download level lit as a green→amber→red VU column and the upload level
    // marked by a blue cap dot per column.
    const QRect area = rect();
    painter.fillRect(area, Qt::black);

    constexpr int kCell = 7;                 // dot pitch in px
    const double kRadius = kCell * 0.27;     // dot radius (25% smaller LEDs)
    const int cols = std::max(1, area.width() / kCell);
    const int rows = std::max(1, area.height() / kCell);

    // Center the matrix within the widget.
    const int offsetX = (area.width() - cols * kCell) / 2;
    const int offsetY = (area.height() - rows * kCell) / 2;
    const auto cx = [&](int c) { return offsetX + c * kCell + kCell / 2.0; };
    const auto cy = [&](int r) { // row 0 at the bottom
        return offsetY + (rows - 1 - r) * kCell + kCell / 2.0;
    };

    const QColor kDim(0x10, 0x20, 0x12);     // unlit LED
    const QColor kGreen(0x33, 0xdd, 0x55);
    const QColor kBlue(0x55, 0xbb, 0xff);

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    // Dim matrix backdrop.
    painter.setBrush(kDim);
    for (int c = 0; c < cols; ++c)
        for (int r = 0; r < rows; ++r)
            painter.drawEllipse(QPointF(cx(c), cy(r)), kRadius, kRadius);

    // Message mode: render the text rasterized onto the LED grid, scrolling
    // left. When it has fully scrolled off, fall through to the graph.
    if (mode_ == Mode::Message && !message_.isEmpty()) {
        QFont f = font();
        f.setBold(true);
        f.setPixelSize(std::max(4, rows));
        QFontMetrics fm(f);
        int w = fm.horizontalAdvance(message_);
        if (w > cols && w > 0) {
            f.setPixelSize(std::max(4, rows * cols / w));
            fm = QFontMetrics(f);
            w = fm.horizontalAdvance(message_);
        }
        const int x = (cols - w) / 2 - scrollCols_;
        if (x + w >= 0) {
            QImage mask(std::max(1, cols), std::max(1, rows),
                        QImage::Format_ARGB32_Premultiplied);
            mask.fill(Qt::transparent);
            QPainter textPainter(&mask);
            textPainter.setFont(f);
            textPainter.setPen(Qt::white);
            textPainter.drawText(x, (rows - fm.height()) / 2 + fm.ascent(), message_);
            textPainter.end();

            painter.setBrush(kGreen);
            for (int c = 0; c < cols; ++c)
                for (int r = 0; r < rows; ++r)
                    if (qAlpha(mask.pixel(c, rows - 1 - r)) > 110)
                        painter.drawEllipse(QPointF(cx(c), cy(r)), kRadius, kRadius);
            return; // message shown; skip the graph this frame
        }
        mode_ = Mode::Graph; // fully scrolled off
        if (dismissTimer_)
            dismissTimer_->stop();
    }

    const qsizetype n = down_.size();
    quint64 peak = 1;
    for (qsizetype i = 0; i < n; ++i)
        peak = std::max({peak, down_.at(i), up_.at(i)});

    // Trace each series as a line of single lit LEDs: one dot per time column
    // at the LED row matching its value. A 0 reading lands on the bottom row, so
    // every sampled interval still shows a baseline dot.
    const int span = std::max(1, rows - 1);
    const auto rowFor = [&](quint64 v) {
        return std::clamp(
            static_cast<int>(std::lround(double(v) / double(peak) * span)), 0, rows - 1);
    };

    const qsizetype first = std::max<qsizetype>(0, n - cols);
    for (qsizetype i = first; i < n; ++i) {
        // Right-align: the newest sample sits at the rightmost column and older
        // ones scroll left.
        const int c = cols - static_cast<int>(n - i);
        painter.setBrush(kGreen);
        painter.drawEllipse(QPointF(cx(c), cy(rowFor(down_.at(i)))), kRadius, kRadius);
        if (up_.at(i) > 0) {
            painter.setBrush(kBlue);
            painter.drawEllipse(QPointF(cx(c), cy(rowFor(up_.at(i)))), kRadius, kRadius);
        }
    }

    // LED-style legend.
    const quint64 curDown = down_.isEmpty() ? 0 : down_.last();
    const quint64 curUp = up_.isEmpty() ? 0 : up_.last();
    painter.setPen(kGreen);
    painter.drawText(area.adjusted(6, 3, -6, -3), Qt::AlignTop | Qt::AlignLeft,
                     QStringLiteral("↓ %1   ↑ %2")
                         .arg(humanRate(curDown), humanRate(curUp)));
    painter.setPen(QColor(0x66, 0x88, 0x66));
    painter.drawText(area.adjusted(6, 3, -6, -3), Qt::AlignTop | Qt::AlignRight,
                     QStringLiteral("peak %1").arg(humanRate(peak)));
}

} // namespace amule
