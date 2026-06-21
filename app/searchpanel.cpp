#include "searchpanel.h"

#include <algorithm>

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTableView>
#include <QTabWidget>
#include <QVBoxLayout>

#include "searchfilterproxy.h"
#include "searchresultmodel.h"

namespace amule {

SearchPanel::SearchPanel(QWidget* parent) : QWidget(parent) {
    saved_ = loadSavedSearches();

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(buildControls());

    progress_ = new QProgressBar;
    progress_->setRange(0, 100);
    progress_->setVisible(false);
    layout->addWidget(progress_);

    tabs_ = new QTabWidget;
    tabs_->setTabsClosable(true);
    tabs_->setDocumentMode(true);
    layout->addWidget(tabs_, 1);

    connect(tabs_, &QTabWidget::tabCloseRequested, this, &SearchPanel::onTabClose);
    connect(tabs_, &QTabWidget::currentChanged, this, &SearchPanel::onCurrentTabChanged);

    refreshSavedCombo();
    updateDownloadEnabled();
}

QWidget* SearchPanel::buildControls() {
    auto* container = new QWidget;
    auto* outer = new QVBoxLayout(container);
    outer->setContentsMargins(0, 0, 0, 0);

    // Query row.
    auto* queryRow = new QHBoxLayout;
    queryEdit_ = new QLineEdit;
    queryEdit_->setPlaceholderText(tr("search terms"));
    networkCombo_ = new QComboBox;
    networkCombo_->addItem(tr("Global ed2k"), int(SearchKind::Global));
    networkCombo_->addItem(tr("Kad"), int(SearchKind::Kad));
    networkCombo_->addItem(tr("Local"), int(SearchKind::Local));
    typeCombo_ = new QComboBox;
    typeCombo_->addItem(tr("Any type"), QString());
    typeCombo_->addItem(tr("Audio"), QStringLiteral("Audio"));
    typeCombo_->addItem(tr("Video"), QStringLiteral("Video"));
    typeCombo_->addItem(tr("Image"), QStringLiteral("Image"));
    typeCombo_->addItem(tr("Document"), QStringLiteral("Doc"));
    typeCombo_->addItem(tr("Program"), QStringLiteral("Pro"));
    typeCombo_->addItem(tr("Archive"), QStringLiteral("Arc"));
    searchBtn_ = new QPushButton(tr("Search"));
    downloadBtn_ = new QPushButton(tr("Download"));
    queryRow->addWidget(queryEdit_, 2);
    queryRow->addWidget(networkCombo_);
    queryRow->addWidget(typeCombo_);
    queryRow->addWidget(searchBtn_);
    queryRow->addWidget(downloadBtn_);
    outer->addLayout(queryRow);

    // Filter row.
    auto* filterRow = new QHBoxLayout;
    filterEdit_ = new QLineEdit;
    filterEdit_->setPlaceholderText(tr("name filter"));
    negCheck_ = new QCheckBox(tr("neg"));
    globCheck_ = new QCheckBox(tr("glob"));
    regexCheck_ = new QCheckBox(tr("regex"));
    onlyCompleteCheck_ = new QCheckBox(tr("complete only"));
    minSrcSpin_ = new QSpinBox;
    minSrcSpin_->setRange(0, 1'000'000);
    minSrcSpin_->setSpecialValueText(tr("any"));
    maxSrcSpin_ = new QSpinBox;
    maxSrcSpin_->setRange(0, 1'000'000);
    maxSrcSpin_->setSpecialValueText(tr("any"));
    filterRow->addWidget(new QLabel(tr("Filter:")));
    filterRow->addWidget(filterEdit_, 2);
    filterRow->addWidget(negCheck_);
    filterRow->addWidget(globCheck_);
    filterRow->addWidget(regexCheck_);
    filterRow->addWidget(onlyCompleteCheck_);
    filterRow->addWidget(new QLabel(tr("min")));
    filterRow->addWidget(minSrcSpin_);
    filterRow->addWidget(new QLabel(tr("max")));
    filterRow->addWidget(maxSrcSpin_);
    anchorCheck_ = new QCheckBox(tr("anchor selected"));
    filterRow->addWidget(anchorCheck_);
    outer->addLayout(filterRow);

    // Saved-search row.
    auto* savedRow = new QHBoxLayout;
    savedCombo_ = new QComboBox;
    loadBtn_ = new QPushButton(tr("Load"));
    saveBtn_ = new QPushButton(tr("Save…"));
    deleteBtn_ = new QPushButton(tr("Delete"));
    savedRow->addWidget(new QLabel(tr("Saved:")));
    savedRow->addWidget(savedCombo_, 1);
    savedRow->addWidget(loadBtn_);
    savedRow->addWidget(saveBtn_);
    savedRow->addWidget(deleteBtn_);
    outer->addLayout(savedRow);

    // Wiring.
    connect(searchBtn_, &QPushButton::clicked, this, &SearchPanel::onSearch);
    connect(queryEdit_, &QLineEdit::returnPressed, this, &SearchPanel::onSearch);
    connect(downloadBtn_, &QPushButton::clicked, this, &SearchPanel::onDownload);

    connect(filterEdit_, &QLineEdit::textChanged, this, &SearchPanel::onFilterChanged);
    connect(negCheck_, &QCheckBox::toggled, this, &SearchPanel::onFilterChanged);
    connect(onlyCompleteCheck_, &QCheckBox::toggled, this, &SearchPanel::onFilterChanged);
    connect(minSrcSpin_, &QSpinBox::valueChanged, this, &SearchPanel::onFilterChanged);
    connect(maxSrcSpin_, &QSpinBox::valueChanged, this, &SearchPanel::onFilterChanged);
    // glob and regex are mutually exclusive.
    connect(globCheck_, &QCheckBox::toggled, this, [this](bool on) {
        if (on)
            regexCheck_->setChecked(false);
        onFilterChanged();
    });
    connect(regexCheck_, &QCheckBox::toggled, this, [this](bool on) {
        if (on)
            globCheck_->setChecked(false);
        onFilterChanged();
    });

    connect(loadBtn_, &QPushButton::clicked, this, &SearchPanel::onLoadSaved);
    connect(saveBtn_, &QPushButton::clicked, this, &SearchPanel::onSaveSearch);
    connect(deleteBtn_, &QPushButton::clicked, this, &SearchPanel::onDeleteSaved);

    connect(anchorCheck_, &QCheckBox::toggled, this,
            [this] { updateAnchor(currentResultView()); });

    return container;
}

QTableView* SearchPanel::makeResultTab(const QString& label) {
    auto* view = new QTableView;
    // Parent the model/proxy to the view so closing the tab frees them.
    auto* model = new SearchResultModel(view);
    auto* proxy = new SearchResultFilterProxy(view);
    proxy->setSourceModel(model);
    proxy->setSortRole(Qt::UserRole);
    proxy->setSearchFilter(currentFilter());
    view->setModel(proxy);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    // Extended selection: click = single, Ctrl+click = toggle, Shift+click = range.
    view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->setAlternatingRowColors(true);
    view->setSortingEnabled(true);
    view->setShowGrid(false);
    view->verticalHeader()->setVisible(false);
    auto* header = view->horizontalHeader();
    header->setSectionResizeMode(SearchResultModel::Name, QHeaderView::Stretch);
    header->setSectionResizeMode(SearchResultModel::Size, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(SearchResultModel::Sources, QHeaderView::ResizeToContents);
    view->setProperty("baseLabel", label);
    // Default sort by source count (descending) so anchoring has an order to
    // work against and results start most-available-first.
    view->sortByColumn(SearchResultModel::Sources, Qt::DescendingOrder);
    view->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(view->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this, view] {
                updateDownloadEnabled();
                updateAnchor(view);
            });
    connect(view, &QTableView::doubleClicked, this, &SearchPanel::onDownload);
    connect(view, &QTableView::customContextMenuRequested, this,
            [this, view](const QPoint& pos) { onResultContextMenu(view, pos); });

    const int index = tabs_->addTab(view, label);
    tabs_->setCurrentIndex(index);
    return view;
}

void SearchPanel::onSearch() {
    const QString text = queryEdit_->text().trimmed();
    if (text.isEmpty())
        return;

    SearchParams params;
    params.text = text;
    params.kind = static_cast<SearchKind>(networkCombo_->currentData().toInt());
    params.fileType = typeCombo_->currentData().toString();

    // The new tab becomes the live (most-recent) one.
    liveView_ = makeResultTab(text);
    liveModel_ = modelFor(liveView_);
    updateDownloadEnabled();
    emit searchRequested(params);
}

void SearchPanel::setResults(const QList<SearchResult>& results) {
    if (!liveModel_ || !liveView_)
        return;
    liveModel_->setResults(results);
    const int index = tabs_->indexOf(liveView_);
    if (index >= 0) {
        const QString base = liveView_->property("baseLabel").toString();
        tabs_->setTabText(index, QStringLiteral("%1 (%2)").arg(base).arg(results.size()));
    }
}

void SearchPanel::setProgress(quint32 progress) {
    progress_->setValue(static_cast<int>(progress));
}

void SearchPanel::setSearching(bool searching) {
    progress_->setVisible(searching);
}

void SearchPanel::onDownload() {
    QTableView* view = currentResultView();
    if (view != liveView_)
        return; // only the live tab's results are in the daemon
    auto* proxy = proxyFor(view);
    auto* model = modelFor(view);
    if (!proxy || !model)
        return;
    // Download every selected result.
    for (const QModelIndex& idx : view->selectionModel()->selectedRows()) {
        const Hash16 hash = model->hashAt(proxy->mapToSource(idx).row());
        if (hash != Hash16{})
            emit downloadRequested(hash);
    }
}

void SearchPanel::onResultContextMenu(QTableView* view, const QPoint& pos) {
    const QModelIndex idx = view->indexAt(pos);
    if (!idx.isValid())
        return;
    auto* proxy = proxyFor(view);
    auto* model = modelFor(view);
    if (!proxy || !model)
        return;
    const SearchResult clicked = model->resultAt(proxy->mapToSource(idx).row());
    const bool live = (view == liveView_);

    QMenu menu(this);
    QAction* detailAct = menu.addAction(tr("Detail"));
    QAction* downloadAct = menu.addAction(tr("Download"));
    QAction* downloadSelAct = menu.addAction(tr("Download selected"));
    // Only the live tab's results are in the daemon and downloadable.
    downloadAct->setEnabled(live);
    downloadSelAct->setEnabled(live && view->selectionModel()->hasSelection());

    QAction* chosen = menu.exec(view->viewport()->mapToGlobal(pos));
    if (chosen == detailAct) {
        showResultDetail(clicked);
    } else if (chosen == downloadAct) {
        if (clicked.hash != Hash16{})
            emit downloadRequested(clicked.hash);
    } else if (chosen == downloadSelAct) {
        onDownload(); // downloads every selected row (live tab)
    }
}

void SearchPanel::showResultDetail(const SearchResult& result) {
    const QByteArray hashBytes(reinterpret_cast<const char*>(result.hash.data()), 16);
    const QString text =
        tr("Name:\n%1\n\nSize: %2\nSources: %3 (complete %4)\nHash: %5")
            .arg(result.name, humanBytes(result.size),
                 QString::number(result.sourceCount),
                 QString::number(result.completeSourceCount),
                 QString::fromLatin1(hashBytes.toHex()));
    QMessageBox::information(this, tr("Result detail"), text);
}

void SearchPanel::updateAnchor(QTableView* view) {
    if (!view)
        return;
    auto* proxy = proxyFor(view);
    if (!proxy)
        return;
    const bool enabled = anchorCheck_->isChecked();
    QSet<int> rows;
    if (enabled) {
        for (const QModelIndex& idx : view->selectionModel()->selectedRows())
            rows.insert(proxy->mapToSource(idx).row());
    }
    proxy->setAnchored(rows, enabled);
}

void SearchPanel::onTabClose(int index) {
    QWidget* page = tabs_->widget(index);
    if (page == liveView_) {
        emit stopRequested();
        liveView_ = nullptr;
        liveModel_ = nullptr;
    }
    tabs_->removeTab(index);
    delete page;
    updateDownloadEnabled();
}

void SearchPanel::onCurrentTabChanged(int) {
    applyFilterToCurrent();
    updateDownloadEnabled();
    updateAnchor(currentResultView());
}

void SearchPanel::onFilterChanged() {
    applyFilterToCurrent();
}

void SearchPanel::applyFilterToCurrent() {
    if (QTableView* view = currentResultView()) {
        if (auto* proxy = proxyFor(view))
            proxy->setSearchFilter(currentFilter());
    }
}

SearchFilter SearchPanel::currentFilter() const {
    SearchFilter f;
    f.text = filterEdit_->text();
    f.negate = negCheck_->isChecked();
    f.glob = globCheck_->isChecked();
    f.regex = regexCheck_->isChecked();
    f.onlyComplete = onlyCompleteCheck_->isChecked();
    f.minSources = static_cast<quint64>(minSrcSpin_->value());
    f.maxSources = static_cast<quint64>(maxSrcSpin_->value());
    return f;
}

QTableView* SearchPanel::currentResultView() const {
    return qobject_cast<QTableView*>(tabs_->currentWidget());
}

SearchResultFilterProxy* SearchPanel::proxyFor(QTableView* view) const {
    return view ? qobject_cast<SearchResultFilterProxy*>(view->model()) : nullptr;
}

SearchResultModel* SearchPanel::modelFor(QTableView* view) const {
    auto* proxy = proxyFor(view);
    return proxy ? qobject_cast<SearchResultModel*>(proxy->sourceModel()) : nullptr;
}

Hash16 SearchPanel::selectedHash() const {
    QTableView* view = currentResultView();
    auto* proxy = proxyFor(view);
    auto* model = modelFor(view);
    if (!proxy || !model)
        return {};
    const QModelIndex current = view->selectionModel()->currentIndex();
    if (!current.isValid())
        return {};
    return model->hashAt(proxy->mapToSource(current).row());
}

void SearchPanel::updateDownloadEnabled() {
    QTableView* view = currentResultView();
    const bool isLive = view != nullptr && view == liveView_;
    const bool hasSelection =
        view != nullptr && view->selectionModel()->hasSelection();
    downloadBtn_->setEnabled(isLive && hasSelection);
}

void SearchPanel::onSaveSearch() {
    bool ok = false;
    const QString name = QInputDialog::getText(
        this, tr("Save search"), tr("Name:"),
        QLineEdit::Normal, queryEdit_->text().trimmed(), &ok);
    if (!ok || name.trimmed().isEmpty())
        return;

    SavedSearch s;
    s.name = name.trimmed();
    s.text = queryEdit_->text();
    s.kindIndex = networkCombo_->currentIndex();
    s.typeIndex = typeCombo_->currentIndex();
    s.filterText = filterEdit_->text();
    s.filterNegate = negCheck_->isChecked();
    s.filterGlob = globCheck_->isChecked();
    s.filterRegex = regexCheck_->isChecked();
    s.onlyComplete = onlyCompleteCheck_->isChecked();
    s.minSources = static_cast<quint64>(minSrcSpin_->value());
    s.maxSources = static_cast<quint64>(maxSrcSpin_->value());

    auto it = std::find_if(saved_.begin(), saved_.end(),
                           [&](const SavedSearch& e) { return e.name == s.name; });
    if (it != saved_.end())
        *it = s;
    else
        saved_.append(s);
    saveSavedSearches(saved_);
    refreshSavedCombo();
    savedCombo_->setCurrentText(s.name);
}

void SearchPanel::onLoadSaved() {
    const int idx = savedCombo_->currentIndex();
    if (idx < 0 || idx >= saved_.size())
        return;
    const SavedSearch s = saved_.at(idx);

    queryEdit_->setText(s.text);
    networkCombo_->setCurrentIndex(std::clamp(s.kindIndex, 0, networkCombo_->count() - 1));
    typeCombo_->setCurrentIndex(std::clamp(s.typeIndex, 0, typeCombo_->count() - 1));
    filterEdit_->setText(s.filterText);
    negCheck_->setChecked(s.filterNegate);
    globCheck_->setChecked(s.filterGlob);
    regexCheck_->setChecked(s.filterRegex);
    onlyCompleteCheck_->setChecked(s.onlyComplete);
    minSrcSpin_->setValue(static_cast<int>(s.minSources));
    maxSrcSpin_->setValue(static_cast<int>(s.maxSources));

    onSearch();
}

void SearchPanel::onDeleteSaved() {
    const int idx = savedCombo_->currentIndex();
    if (idx < 0 || idx >= saved_.size())
        return;
    saved_.removeAt(idx);
    saveSavedSearches(saved_);
    refreshSavedCombo();
}

void SearchPanel::refreshSavedCombo() {
    savedCombo_->clear();
    for (const SavedSearch& s : saved_)
        savedCombo_->addItem(s.name);
    const bool any = !saved_.isEmpty();
    loadBtn_->setEnabled(any);
    deleteBtn_->setEnabled(any);
}

} // namespace amule
