// Item delegate that paints a progress bar from DownloadTableModel::ProgressRole.

#pragma once

#include <QStyledItemDelegate>

namespace amule {

class ProgressBarDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
};

} // namespace amule
