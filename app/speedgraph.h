// A compact in-memory speed graph: a rolling area chart of download and upload
// rates, fed one sample per worker refresh (~1 s). No persistence (the on-disk
// speed history is a deferred enhancement).

#pragma once

#include <QList>
#include <QWidget>

#include <QtTypes>

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

public slots:
    void addSample(quint64 downBytesPerSec, quint64 upBytesPerSec);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    static constexpr int kMaxSamples = 180; // ~3 min at 1 Hz

    QList<quint64> down_;
    QList<quint64> up_;
};

} // namespace amule
