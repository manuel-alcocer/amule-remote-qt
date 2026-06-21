// A compact in-memory speed graph: a rolling area chart of download and upload
// rates, fed one sample per worker refresh (~1 s). No persistence (the on-disk
// speed history is a deferred enhancement).

#pragma once

#include <QList>
#include <QString>
#include <QWidget>

#include <QtTypes>

class QTimer;

namespace amule {

class SpeedGraph : public QWidget {
    Q_OBJECT

public:
    explicit SpeedGraph(QWidget* parent = nullptr);

    [[nodiscard]] QSize sizeHint() const override;

    // Persist / restore the in-memory sample buffers so the graph resumes after
    // a restart. Best-effort: I/O failures are ignored silently.
    void save(const QString& path) const;
    void load(const QString& path);

    // Show a scrolling LED message instead of the graph (e.g. a welcome banner).
    void showMessage(const QString& text);
    // Scroll the current message off to the left, then reveal the graph.
    void dismissMessage();

public slots:
    void addSample(quint64 downBytesPerSec, quint64 upBytesPerSec);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    static constexpr int kMaxSamples = 180; // ~3 min at 1 Hz

    enum class Mode { Graph, Message };

    QList<quint64> down_;
    QList<quint64> up_;

    Mode mode_ = Mode::Graph;
    QString message_;
    int scrollCols_ = 0;        // how far the message has scrolled left, in LEDs
    QTimer* dismissTimer_ = nullptr;
};

} // namespace amule
