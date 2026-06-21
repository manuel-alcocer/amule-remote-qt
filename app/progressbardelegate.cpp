#include "progressbardelegate.h"

#include <QApplication>
#include <QPainter>
#include <QStyleOptionProgressBar>

#include "downloadtablemodel.h"

namespace amule {

void ProgressBarDelegate::paint(QPainter* painter,
                                const QStyleOptionViewItem& option,
                                const QModelIndex& index) const {
    const int percent = index.data(DownloadTableModel::ProgressRole).toInt();

    QStyleOptionProgressBar bar;
    bar.rect = option.rect.adjusted(3, 3, -3, -3);
    bar.minimum = 0;
    bar.maximum = 100;
    bar.progress = percent;
    bar.text = QStringLiteral("%1%").arg(percent);
    bar.textVisible = true;
    bar.textAlignment = Qt::AlignCenter;
    bar.state = option.state;

    QApplication::style()->drawControl(QStyle::CE_ProgressBar, &bar, painter);
}

} // namespace amule
