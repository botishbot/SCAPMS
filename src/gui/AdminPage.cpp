#include "gui/AdminPage.h"

#include <QAbstractItemView>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace scapms::gui {

AdminPage::AdminPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);
    auto* title = new QLabel(tr("Administration"), this);
    title->setObjectName("PageTitle");
    root->addWidget(title);

    auto* grid = new QGridLayout();
    grid->setSpacing(16);

    // ---- data file ----
    auto* fileCard = new QFrame(this);
    fileCard->setObjectName("Card");
    auto* fileLay = new QVBoxLayout(fileCard);
    fileLay->setContentsMargins(20, 16, 20, 16);
    fileLay->setSpacing(8);
    auto* fileTitle = new QLabel(tr("Data file"), fileCard);
    fileTitle->setObjectName("SectionTitle");
    m_path = new QLabel(fileCard);
    m_path->setObjectName("FilePathLabel");
    m_path->setWordWrap(true);
    m_path->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_status = new QLabel(fileCard);
    m_status->setObjectName("FileStatusLabel");
    fileLay->addWidget(fileTitle);
    fileLay->addWidget(m_path);
    fileLay->addWidget(m_status);

    auto addButton = [&](QBoxLayout* lay, const QString& text, const QString& tip, bool primary = false, bool danger = false) {
        auto* b = new QPushButton(text, fileCard);
        b->setToolTip(tip);
        if (primary) b->setObjectName("Primary");
        if (danger) b->setObjectName("Danger");
        lay->addWidget(b);
        return b;
    };
    auto* row1 = new QHBoxLayout();
    auto* saveBtn = addButton(row1, tr("Save"), tr("Save all data to the data file (Ctrl+S)"), true);
    auto* saveAsBtn = addButton(row1, tr("Save As..."), tr("Save to a file you choose (Ctrl+Shift+S)"));
    auto* loadBtn = addButton(row1, tr("Load..."), tr("Load data from a file you choose (Ctrl+O)"));
    auto* row2 = new QHBoxLayout();
    auto* exportBtn = addButton(row2, tr("Export patients (CSV)..."), tr("Write the patient list to a spreadsheet file"));
    auto* newBtn = addButton(row2, tr("New (clear all data)"), tr("Start with an empty clinic (Ctrl+N)"), false, true);
    fileLay->addLayout(row1);
    fileLay->addLayout(row2);
    fileLay->addStretch(1);
    grid->addWidget(fileCard, 0, 0);

    // ---- demo data ----
    auto* demoCard = new QFrame(this);
    demoCard->setObjectName("Card");
    auto* demoLay = new QVBoxLayout(demoCard);
    demoLay->setContentsMargins(20, 16, 20, 16);
    demoLay->setSpacing(8);
    auto* demoTitle = new QLabel(tr("Demo data"), demoCard);
    demoTitle->setObjectName("SectionTitle");
    auto* demoText = new QLabel(tr("Fills the clinic with 30 fictional patients, with services and appointments, so you can "
                                   "try every feature. If there is data already, you are asked before it is replaced. "
                                   "Demo data is never mixed with real records."), demoCard);
    demoText->setObjectName("Muted");
    demoText->setWordWrap(true);
    auto* demoBtn = new QPushButton(tr("Load demo data"), demoCard);
    demoBtn->setToolTip(tr("Replace the current data with fictional sample data"));
    demoLay->addWidget(demoTitle);
    demoLay->addWidget(demoText);
    demoLay->addWidget(demoBtn);
    demoLay->addStretch(1);
    grid->addWidget(demoCard, 0, 1);

    // ---- undo history ----
    auto* undoCard = new QFrame(this);
    undoCard->setObjectName("Card");
    auto* undoLay = new QVBoxLayout(undoCard);
    undoLay->setContentsMargins(20, 16, 20, 16);
    undoLay->setSpacing(8);
    auto* undoTitle = new QLabel(tr("Undo history (newest first)"), undoCard);
    undoTitle->setObjectName("SectionTitle");
    m_undoList = new QListWidget(undoCard);
    m_undoList->setObjectName("UndoHistoryList");
    m_undoList->setProperty("panel", true);
    m_undoList->setSelectionMode(QAbstractItemView::NoSelection);
    m_undoList->setFocusPolicy(Qt::NoFocus);
    m_undoBtn = new QPushButton(tr("Undo last operation"), undoCard);
    m_undoBtn->setToolTip(tr("Reverse the most recent administrative operation (Ctrl+Z)"));
    undoLay->addWidget(undoTitle);
    undoLay->addWidget(m_undoList, 1);
    undoLay->addWidget(m_undoBtn);
    grid->addWidget(undoCard, 1, 0);

    // ---- summary ----
    auto* sumCard = new QFrame(this);
    sumCard->setObjectName("Card");
    auto* sumLay = new QVBoxLayout(sumCard);
    sumLay->setContentsMargins(20, 16, 20, 16);
    sumLay->setSpacing(8);
    auto* sumTitle = new QLabel(tr("Data summary"), sumCard);
    sumTitle->setObjectName("SectionTitle");
    m_summary = new QLabel(sumCard);
    m_summary->setObjectName("SummaryLabel");
    m_summary->setWordWrap(true);
    auto* note = new QLabel(tr("The waiting queue is not saved: it starts empty each time the data is loaded."), sumCard);
    note->setObjectName("Muted");
    note->setWordWrap(true);
    sumLay->addWidget(sumTitle);
    sumLay->addWidget(m_summary);
    sumLay->addWidget(note);
    sumLay->addStretch(1);
    grid->addWidget(sumCard, 1, 1);

    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    grid->setRowStretch(1, 1);
    root->addLayout(grid, 1);

    connect(saveBtn, &QPushButton::clicked, this, &AdminPage::saveRequested);
    connect(saveAsBtn, &QPushButton::clicked, this, &AdminPage::saveAsRequested);
    connect(loadBtn, &QPushButton::clicked, this, &AdminPage::loadRequested);
    connect(exportBtn, &QPushButton::clicked, this, &AdminPage::exportRequested);
    connect(newBtn, &QPushButton::clicked, this, &AdminPage::newRequested);
    connect(demoBtn, &QPushButton::clicked, this, &AdminPage::demoRequested);
    connect(m_undoBtn, &QPushButton::clicked, this, &AdminPage::undoRequested);
    setInfo(AdminInfo());
}

void AdminPage::setInfo(const AdminInfo& info) {
    m_path->setText(info.path.isEmpty() ? tr("No data file yet. Save will ask where to save.")
                                        : tr("File: %1").arg(info.path));
    if (info.dirty) m_status->setText(tr("Unsaved changes"));
    else if (info.path.isEmpty()) m_status->setText(tr("Nothing saved yet"));
    else m_status->setText(tr("All changes saved"));
    m_status->setStyleSheet(info.dirty ? "color:#93370D; font-weight:600;" : "color:#05603A; font-weight:600;");

    m_undoList->clear();
    if (info.undoLines.isEmpty()) {
        auto* item = new QListWidgetItem(tr("Nothing to undo"));
        item->setFlags(Qt::NoItemFlags);
        m_undoList->addItem(item);
    } else {
        m_undoList->addItems(info.undoLines);
    }
    m_undoBtn->setEnabled(!info.undoLines.isEmpty());

    m_summary->setText(tr("%1 patient(s)\n%2 appointment(s) on live patients\n%3 service(s)\n"
                          "%4 appointment(s) kept as history from deleted patients")
                           .arg(info.patients).arg(info.appointments).arg(info.services).arg(info.history));
}

} // namespace scapms::gui
