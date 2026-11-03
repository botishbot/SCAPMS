#include "gui/DashboardPage.h"

#include <QAbstractItemView>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QProgressBar>
#include <QVBoxLayout>

namespace scapms::gui {

DashboardPage::DashboardPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto* title = new QLabel(tr("Dashboard"), this);
    title->setObjectName("PageTitle");
    root->addWidget(title);

    auto* cards = new QGridLayout();
    cards->setSpacing(16);
    cards->addWidget(makeCard(tr("Registered patients"), m_patients), 0, 0);
    cards->addWidget(makeCard(tr("Appointments today"), m_today), 0, 1);
    cards->addWidget(makeCard(tr("Pending appointments"), m_pending), 0, 2);
    cards->addWidget(makeCard(tr("Waiting now"), m_waiting), 0, 3);
    root->addLayout(cards);

    auto* panels = new QGridLayout();
    panels->setSpacing(16);
    panels->addWidget(makePanel(tr("Today's appointments"), m_todayList, "TodayList"), 0, 0);
    panels->addWidget(makePanel(tr("Waiting queue"), m_queueList, "QueueList"), 0, 1);
    panels->addWidget(makeDistributionPanel(), 1, 0);
    panels->addWidget(makePanel(tr("Frequently accessed patients"), m_frequentList, "FrequentList"), 1, 1);
    panels->setRowStretch(0, 1);
    panels->setRowStretch(1, 1);
    panels->setColumnStretch(0, 1);
    panels->setColumnStretch(1, 1);
    root->addLayout(panels, 1);

    setData(DashboardData());
}

QWidget* DashboardPage::makeCard(const QString& caption, QLabel*& valueLabel) {
    auto* card = new QFrame(this);
    card->setObjectName("Card");
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(20, 16, 20, 16);
    lay->setSpacing(4);
    valueLabel = new QLabel("0", card);
    valueLabel->setObjectName("CardValue");
    auto* cap = new QLabel(caption, card);
    cap->setObjectName("CardLabel");
    lay->addWidget(valueLabel);
    lay->addWidget(cap);
    return card;
}

QWidget* DashboardPage::makePanel(const QString& title, QListWidget*& list, const char* objectName) {
    auto* card = new QFrame(this);
    card->setObjectName("Card");
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(20, 14, 20, 14);
    lay->setSpacing(6);
    auto* t = new QLabel(title, card);
    t->setObjectName("SectionTitle");
    list = new QListWidget(card);
    list->setObjectName(objectName);
    list->setProperty("panel", true);
    list->setSelectionMode(QAbstractItemView::NoSelection);
    list->setFocusPolicy(Qt::NoFocus);
    list->setFrameShape(QFrame::NoFrame);
    lay->addWidget(t);
    lay->addWidget(list, 1);
    return card;
}

QWidget* DashboardPage::makeDistributionPanel() {
    auto* card = new QFrame(this);
    card->setObjectName("Card");
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(20, 14, 20, 14);
    lay->setSpacing(10);
    auto* t = new QLabel(tr("Patient categories"), card);
    t->setObjectName("SectionTitle");
    lay->addWidget(t);

    auto addRow = [&](const QString& name, const char* barName, QProgressBar*& bar, QLabel*& count) {
        auto* row = new QGridLayout();
        row->setHorizontalSpacing(12);
        auto* label = new QLabel(name, card);
        label->setMinimumWidth(90);
        bar = new QProgressBar(card);
        bar->setObjectName(barName);
        bar->setTextVisible(false);
        bar->setRange(0, 100);
        bar->setFixedHeight(12);
        count = new QLabel("0", card);
        count->setMinimumWidth(28);
        count->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row->addWidget(label, 0, 0);
        row->addWidget(bar, 0, 1);
        row->addWidget(count, 0, 2);
        row->setColumnStretch(1, 1);
        lay->addLayout(row);
    };
    addRow(tr("General"), "BarGeneral", m_barGeneral, m_countGeneral);
    addRow(tr("Emergency"), "BarEmergency", m_barEmergency, m_countEmergency);
    addRow(tr("Follow-Up"), "BarFollowUp", m_barFollowUp, m_countFollowUp);
    lay->addStretch(1);
    return card;
}

void DashboardPage::fillList(QListWidget* list, const QStringList& lines, const QString& emptyText) {
    list->clear();
    if (lines.isEmpty()) {
        auto* item = new QListWidgetItem(emptyText);
        item->setFlags(Qt::NoItemFlags);
        list->addItem(item);
        return;
    }
    list->addItems(lines);
}

void DashboardPage::setData(const DashboardData& d) {
    m_patients->setText(QString::number(d.patients));
    m_today->setText(QString::number(d.todayAppointments));
    m_pending->setText(QString::number(d.pending));
    m_waiting->setText(QString::number(d.waiting));

    fillList(m_todayList, d.todayLines, tr("No appointments today"));
    fillList(m_queueList, d.queueLines, tr("Nobody is waiting"));
    fillList(m_frequentList, d.frequentLines, tr("Open patient records to see them here"));

    const int total = d.general + d.emergency + d.followUp;
    auto setBar = [&](QProgressBar* bar, QLabel* label, int n) {
        bar->setValue(total > 0 ? (n * 100 + total / 2) / total : 0);  // rounded to the nearest percent
        label->setText(QString::number(n));
    };
    setBar(m_barGeneral, m_countGeneral, d.general);
    setBar(m_barEmergency, m_countEmergency, d.emergency);
    setBar(m_barFollowUp, m_countFollowUp, d.followUp);
}

} // namespace scapms::gui
