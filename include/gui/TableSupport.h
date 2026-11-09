#pragma once

#include <QHeaderView>
#include <QSettings>
#include <QSignalBlocker>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QTableWidget>
#include <QVariant>

namespace scapms::gui {

namespace {
class CenteredTableItemDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

protected:
    void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override {
        QStyledItemDelegate::initStyleOption(option, index);
        option->displayAlignment = Qt::AlignCenter;
    }
};
} // namespace

// Shared table behaviour for every data table in SCAPMS.
// Widths are user-adjustable and persisted independently by the table's object name.
inline void configureResizableTable(QTableWidget* table,
                                    const QList<int>& preferredWidths,
                                    const QList<int>& minimumWidths = {},
                                    int maximumWidth = 560) {
    if (!table || !table->horizontalHeader()) return;

    auto* header = table->horizontalHeader();
    const int columns = table->columnCount();

    // Apply one alignment rule to every cell and every header in every shared table.
    // A delegate changes presentation only; it does not alter stored values or table behaviour.
    table->setItemDelegate(new CenteredTableItemDelegate(table));
    header->setDefaultAlignment(Qt::AlignCenter);
    QList<int> minimums;
    minimums.reserve(columns);
    for (int column = 0; column < columns; ++column) {
        const int minimum = column < minimumWidths.size()
                                ? minimumWidths.at(column)
                                : 72;
        minimums.push_back(qBound(48, minimum, maximumWidth));
    }

    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    table->setWordWrap(false);
    table->setTextElideMode(Qt::ElideRight);
    header->setStretchLastSection(false);
    header->setSectionsMovable(false);
    header->setHighlightSections(false);
    header->setMinimumSectionSize(48);
    header->setDefaultSectionSize(120);

    const QString settingsKey = QStringLiteral("tableColumns/") +
                                (table->objectName().isEmpty()
                                     ? QStringLiteral("unnamed")
                                     : table->objectName());

    QSettings settings;
    settings.beginGroup(settingsKey);
    for (int column = 0; column < columns; ++column) {
        header->setSectionResizeMode(column, QHeaderView::Interactive);
        const int preferred = column < preferredWidths.size()
                                  ? preferredWidths.at(column)
                                  : 120;
        const int saved = settings.value(QString::number(column), preferred).toInt();
        header->resizeSection(column, qBound(minimums.at(column), saved, maximumWidth));
    }
    settings.endGroup();

    QObject::connect(header, &QHeaderView::sectionResized, table,
                     [header, table, minimums, maximumWidth, settingsKey](int logicalIndex,
                                                                          int,
                                                                          int newSize) {
        if (logicalIndex < 0 || logicalIndex >= minimums.size()) return;
        const int bounded = qBound(minimums.at(logicalIndex), newSize, maximumWidth);
        if (bounded != newSize) {
            const QSignalBlocker blocker(header);
            header->resizeSection(logicalIndex, bounded);
        }

        QSettings currentSettings;
        currentSettings.beginGroup(settingsKey);
        for (int column = 0; column < table->columnCount(); ++column)
            currentSettings.setValue(QString::number(column), header->sectionSize(column));
        currentSettings.endGroup();
    });
}

} // namespace scapms::gui
