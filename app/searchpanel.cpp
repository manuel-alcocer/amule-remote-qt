#include "searchpanel.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QVBoxLayout>

#include "searchresultmodel.h"

namespace amule {

SearchPanel::SearchPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);

    // Query controls.
    auto* controls = new QHBoxLayout;
    queryEdit_ = new QLineEdit;
    queryEdit_->setPlaceholderText(QStringLiteral("search terms"));

    networkCombo_ = new QComboBox;
    networkCombo_->addItem(QStringLiteral("Global ed2k"),
                           int(SearchKind::Global));
    networkCombo_->addItem(QStringLiteral("Kad"), int(SearchKind::Kad));
    networkCombo_->addItem(QStringLiteral("Local"), int(SearchKind::Local));

    typeCombo_ = new QComboBox;
    typeCombo_->addItem(QStringLiteral("Any type"), QString());
    typeCombo_->addItem(QStringLiteral("Audio"), QStringLiteral("Audio"));
    typeCombo_->addItem(QStringLiteral("Video"), QStringLiteral("Video"));
    typeCombo_->addItem(QStringLiteral("Image"), QStringLiteral("Image"));
    typeCombo_->addItem(QStringLiteral("Document"), QStringLiteral("Doc"));
    typeCombo_->addItem(QStringLiteral("Program"), QStringLiteral("Pro"));
    typeCombo_->addItem(QStringLiteral("Archive"), QStringLiteral("Arc"));

    searchBtn_ = new QPushButton(QStringLiteral("Search"));
    downloadBtn_ = new QPushButton(QStringLiteral("Download"));
    downloadBtn_->setEnabled(false);

    controls->addWidget(queryEdit_, 2);
    controls->addWidget(networkCombo_);
    controls->addWidget(typeCombo_);
    controls->addWidget(searchBtn_);
    controls->addWidget(downloadBtn_);
    layout->addLayout(controls);

    progress_ = new QProgressBar;
    progress_->setRange(0, 100);
    progress_->setVisible(false);
    layout->addWidget(progress_);

    // Results.
    model_ = new SearchResultModel(this);
    proxy_ = new QSortFilterProxyModel(this);
    proxy_->setSourceModel(model_);
    proxy_->setSortRole(Qt::UserRole);

    table_ = new QTableView;
    table_->setModel(proxy_);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->setSortingEnabled(true);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setSectionResizeMode(SearchResultModel::Name,
                                                     QHeaderView::Stretch);
    layout->addWidget(table_, 1);

    connect(searchBtn_, &QPushButton::clicked, this, &SearchPanel::onSearchOrStop);
    connect(queryEdit_, &QLineEdit::returnPressed, this, &SearchPanel::onSearchOrStop);
    connect(downloadBtn_, &QPushButton::clicked, this, &SearchPanel::onDownloadSelected);
    connect(table_, &QTableView::doubleClicked, this, &SearchPanel::onDownloadSelected);
    connect(table_->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this] { downloadBtn_->setEnabled(table_->selectionModel()->hasSelection()); });
}

void SearchPanel::onSearchOrStop() {
    if (searching_) {
        emit stopRequested();
        return;
    }
    const QString text = queryEdit_->text().trimmed();
    if (text.isEmpty())
        return;

    SearchParams params;
    params.text = text;
    params.kind = static_cast<SearchKind>(networkCombo_->currentData().toInt());
    params.fileType = typeCombo_->currentData().toString();
    emit searchRequested(params);
}

void SearchPanel::onDownloadSelected() {
    const Hash16 hash = selectedHash();
    if (hash != Hash16{})
        emit downloadRequested(hash);
}

Hash16 SearchPanel::selectedHash() const {
    const QModelIndex current = table_->selectionModel()->currentIndex();
    if (!current.isValid())
        return {};
    return model_->hashAt(proxy_->mapToSource(current).row());
}

void SearchPanel::setResults(const QList<SearchResult>& results) {
    model_->setResults(results);
}

void SearchPanel::setProgress(quint32 progress) {
    progress_->setValue(static_cast<int>(progress));
}

void SearchPanel::setSearching(bool searching) {
    searching_ = searching;
    searchBtn_->setText(searching ? QStringLiteral("Stop")
                                  : QStringLiteral("Search"));
    progress_->setVisible(searching);
}

} // namespace amule
