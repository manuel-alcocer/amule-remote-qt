// Named, persisted search presets. Each captures the full query form (query,
// network, type) and the client-side filters, so a search can be replayed.
// Stored as a tab-separated file under the app config directory. Ported from
// rstamule-cli's saved_search.rs.

#pragma once

#include <QList>
#include <QString>
#include <QtTypes>

namespace amule {

struct SavedSearch {
    QString name;
    QString text;
    int kindIndex = 0;
    int typeIndex = 0;
    QString filterText;
    bool filterNegate = false;
    bool filterGlob = false;
    bool filterRegex = false;
    bool onlyComplete = false;
    quint64 minSources = 0;
    quint64 maxSources = 0;
};

// Load all saved searches (empty list if none/unreadable).
QList<SavedSearch> loadSavedSearches();

// Persist all saved searches.
void saveSavedSearches(const QList<SavedSearch>& searches);

} // namespace amule
