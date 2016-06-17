/* -*- mode: c++; c-basic-offset:4 -*-
    crypto/gui/resultitemwidget.cpp

    This file is part of Kleopatra, the KDE keymanager
    Copyright (c) 2008 Klarälvdalens Datakonsult AB

    Kleopatra is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Kleopatra is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include <config-kleopatra.h>

#include "resultitemwidget.h"

#include "utils/auditlog.h"
#include "utils/classify.h"
#include "commands/command.h"

#include <libkleo/messagebox.h>

#include <KLocalizedString>
#include <QPushButton>
#include <KStandardGuiItem>
#include "kleopatra_debug.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QStringList>
#include <QUrl>
#include <QVBoxLayout>
#include <KGuiItem>
#include <KColorScheme>

using namespace Kleo;
using namespace Kleo::Crypto;
using namespace Kleo::Crypto::Gui;
using namespace boost;

namespace
{
// TODO move out of here
static QColor colorForVisualCode(Task::Result::VisualCode code)
{
    switch (code) {
    case Task::Result::AllGood:
        return KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::PositiveBackground).color();
    case Task::Result::NeutralError:
    case Task::Result::Warning:
        return KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::NeutralBackground).color();
    case Task::Result::Danger:
        return KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::NegativeBackground).color();
    case Task::Result::NeutralSuccess:
    default:
        return QColor(0x00, 0x80, 0xFF); // light blue
    }
}
static QColor txtColorForVisualCode(Task::Result::VisualCode code)
{
    switch (code) {
    case Task::Result::AllGood:
        return KColorScheme(QPalette::Active, KColorScheme::View).foreground(KColorScheme::PositiveText).color();
    case Task::Result::NeutralError:
    case Task::Result::Warning:
        return KColorScheme(QPalette::Active, KColorScheme::View).foreground(KColorScheme::NeutralText).color();
    case Task::Result::Danger:
        return KColorScheme(QPalette::Active, KColorScheme::View).foreground(KColorScheme::NegativeText).color();
    case Task::Result::NeutralSuccess:
    default:
        return QColor(0xFF, 0xFF, 0xFF); // white
    }
}
}

class ResultItemWidget::Private
{
    ResultItemWidget *const q;
public:
    explicit Private(const shared_ptr<const Task::Result> result, ResultItemWidget *qq) : q(qq), m_result(result), m_detailsLabel(0), m_showDetailsLabel(0), m_closeButton(0)
    {
        assert(m_result);
    }

    void slotLinkActivated(const QString &);
    void updateShowDetailsLabel();

    const shared_ptr<const Task::Result> m_result;
    QLabel *m_detailsLabel;
    QLabel *m_showDetailsLabel;
    QPushButton *m_closeButton;
};

static QUrl auditlog_url_template()
{
    QUrl url(QStringLiteral("kleoresultitem://showauditlog"));
    return url;
}

void ResultItemWidget::Private::updateShowDetailsLabel()
{
    if (!m_showDetailsLabel || !m_detailsLabel) {
        return;
    }

    const bool detailsVisible = m_detailsLabel->isVisible();
    const QString auditLogLink = m_result->auditLog().formatLink(auditlog_url_template());
    m_showDetailsLabel->setText(QStringLiteral("<a href=\"kleoresultitem://toggledetails/\">%1</a><br/>%2").arg(detailsVisible ? i18n("Hide Details") : i18n("Show Details"), auditLogLink));
    m_showDetailsLabel->setAccessibleDescription(detailsVisible ? i18n("Hide Details") : i18n("Show Details"));
}

ResultItemWidget::ResultItemWidget(const shared_ptr<const Task::Result> &result, QWidget *parent, Qt::WindowFlags flags) : QWidget(parent, flags), d(new Private(result, this))
{
    const QColor color = colorForVisualCode(d->m_result->code());
    const QColor txtColor = txtColorForVisualCode(d->m_result->code());
    const QString styleSheet = QStringLiteral("* { background-color: %1; margin: 0px; }"
                                              "QFrame#resultFrame{ border-color: %2; border-style: solid; border-radius: 3px; border-width: 1px }"
                                              "QLabel { color: %3; padding: 5px; border-radius: 3px }").arg(color.name()).arg(color.darker(150).name()).arg(txtColor.name());
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);
    topLayout->setSpacing(0);
    QFrame *frame = new QFrame;
    frame->setObjectName(QStringLiteral("resultFrame"));
    frame->setStyleSheet(styleSheet);
    topLayout->addWidget(frame);
    QVBoxLayout *layout = new QVBoxLayout(frame);
    layout->setMargin(0);
    layout->setSpacing(0);
    QWidget *hbox = new QWidget;
    QHBoxLayout *hlay = new QHBoxLayout(hbox);
    hlay->setMargin(0);
    hlay->setSpacing(0);
    QLabel *overview = new QLabel;
    overview->setWordWrap(true);
    overview->setTextFormat(Qt::RichText);
    overview->setText(d->m_result->overview());
    overview->setFocusPolicy(Qt::StrongFocus);
    overview->setStyleSheet(styleSheet);
    connect(overview, SIGNAL(linkActivated(QString)), this, SLOT(slotLinkActivated(QString)));

    hlay->addWidget(overview, 1, Qt::AlignTop);
    layout->addWidget(hbox);

    const QString details = d->m_result->details();

    d->m_showDetailsLabel = new QLabel;
    connect(d->m_showDetailsLabel, SIGNAL(linkActivated(QString)), this, SLOT(slotLinkActivated(QString)));
    hlay->addWidget(d->m_showDetailsLabel);
    d->m_showDetailsLabel->setVisible(!details.isEmpty());
    d->m_showDetailsLabel->setFocusPolicy(Qt::StrongFocus);
    d->m_showDetailsLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    d->m_showDetailsLabel->setStyleSheet(styleSheet);

    d->m_detailsLabel = new QLabel;
    d->m_detailsLabel->setWordWrap(true);
    d->m_detailsLabel->setTextFormat(Qt::RichText);
    d->m_detailsLabel->setText(details);
    d->m_detailsLabel->setFocusPolicy(Qt::StrongFocus);
    d->m_detailsLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    d->m_detailsLabel->setStyleSheet(styleSheet);
    connect(d->m_detailsLabel, SIGNAL(linkActivated(QString)), this, SLOT(slotLinkActivated(QString)));
    layout->addWidget(d->m_detailsLabel);

    d->m_detailsLabel->setVisible(false);

    d->m_closeButton = new QPushButton;
    KGuiItem::assign(d->m_closeButton, KStandardGuiItem::close());
    d->m_closeButton->setFixedSize(d->m_closeButton->sizeHint());
    connect(d->m_closeButton, &QAbstractButton::clicked, this, &ResultItemWidget::closeButtonClicked);
    layout->addWidget(d->m_closeButton, 0, Qt::AlignRight);
    d->m_closeButton->setVisible(false);

    d->updateShowDetailsLabel();
}

ResultItemWidget::~ResultItemWidget()
{
}

void ResultItemWidget::showCloseButton(bool show)
{
    d->m_closeButton->setVisible(show);
}

bool ResultItemWidget::detailsVisible() const
{
    return d->m_detailsLabel && d->m_detailsLabel->isVisible();
}

bool ResultItemWidget::hasErrorResult() const
{
    return d->m_result->hasError();
}

void ResultItemWidget::Private::slotLinkActivated(const QString &link)
{
    assert(m_result);
    qCDebug(KLEOPATRA_LOG) << "Link activated: " << link;
    if (link.startsWith(QStringLiteral("key:"))) {
        auto split = link.split(QLatin1Char(':'));
        auto fpr = split.value(1);
        if (split.size() == 2 && isFingerprint(fpr)) {
            /* There might be a security consideration here if somehow
             * a short keyid is used in a link and it collides with another.
             * So we additionally check that it really is a fingerprint. */
            auto cmd = Command::commandForQuery(fpr);
            cmd->setParentWId(q->effectiveWinId());
            cmd->start();
        } else {
            qCWarning(KLEOPATRA_LOG) << "key link invalid " << link;
        }
        return;
    }

    const QUrl url(link);
    if (url.host() == QLatin1String("toggledetails")) {
        q->showDetails(!q->detailsVisible());
        return;
    }

    if (url.host() == QLatin1String("showauditlog")) {
        q->showAuditLog();
        return;
    }
    qCWarning(KLEOPATRA_LOG) << "Unexpected link scheme: " << link;
}

void ResultItemWidget::showAuditLog()
{
    MessageBox::auditLog(parentWidget(), d->m_result->auditLog().text());
}

void ResultItemWidget::showDetails(bool show)
{
    if (show == d->m_detailsLabel->isVisible()) {
        return;
    }
    d->m_detailsLabel->setVisible(show);
    d->updateShowDetailsLabel();
    Q_EMIT detailsToggled(show);
}

#include "moc_resultitemwidget.cpp"
