/*  crypto/gui/certificatelineedit.cpp

    This file is part of Kleopatra, the KDE keymanager
    SPDX-FileCopyrightText: 2016 Bundesamt für Sicherheit in der Informationstechnik
    SPDX-FileContributor: Intevation GmbH

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "certificatelineedit.h"

#include <QCompleter>
#include <QPushButton>
#include <QAction>
#include <QSignalBlocker>

#include "kleopatra_debug.h"

#include "commands/detailscommand.h"

#include <Libkleo/KeyCache>
#include <Libkleo/KeyFilter>
#include <Libkleo/KeyListModel>
#include <Libkleo/KeyListSortFilterProxyModel>
#include <Libkleo/Formatting>

#include <KLocalizedString>
#include <KIconLoader>

#include <gpgme++/key.h>

#include <QGpgME/KeyForMailboxJob>
#include <QGpgME/Protocol>

using namespace Kleo;
using namespace Kleo::Dialogs;
using namespace GpgME;

Q_DECLARE_METATYPE(GpgME::Key)

static QStringList s_lookedUpKeys;

namespace
{
class ProxyModel : public KeyListSortFilterProxyModel
{
    Q_OBJECT

public:
    ProxyModel(QObject *parent = nullptr)
        : KeyListSortFilterProxyModel(parent)
    {
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid()) {
            return QVariant();
        }

        switch (role) {
        case Qt::DecorationRole: {
            const auto key = KeyListSortFilterProxyModel::data(index,
                    Kleo::KeyListModelInterface::KeyRole).value<GpgME::Key>();
            Q_ASSERT(!key.isNull());
            if (key.isNull()) {
                return QVariant();
            }
            return Kleo::Formatting::iconForUid(key.userID(0));
        }
        default:
            return KeyListSortFilterProxyModel::data(index, role);
        }
    }
};
} // namespace

CertificateLineEdit::CertificateLineEdit(AbstractKeyListModel *model,
                                         QWidget *parent,
                                         KeyFilter *filter)
    : QLineEdit(parent),
      mFilterModel(new KeyListSortFilterProxyModel(this)),
      mFilter(std::shared_ptr<KeyFilter>(filter)),
      mLineAction(new QAction(this))
{
    setPlaceholderText(i18n("Please enter a name or email address..."));
    setClearButtonEnabled(true);
    addAction(mLineAction, QLineEdit::LeadingPosition);

    auto *completer = new QCompleter(this);
    auto *completeFilterModel = new ProxyModel(completer);
    completeFilterModel->setKeyFilter(mFilter);
    completeFilterModel->setSourceModel(model);
    completer->setModel(completeFilterModel);
    completer->setCompletionColumn(KeyListModelInterface::Summary);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    setCompleter(completer);
    mFilterModel->setSourceModel(model);
    mFilterModel->setFilterKeyColumn(KeyListModelInterface::Summary);
    if (filter) {
        mFilterModel->setKeyFilter(mFilter);
    }

    connect(KeyCache::instance().get(), &Kleo::KeyCache::keyListingDone,
            this, &CertificateLineEdit::updateKey);
    connect(this, &QLineEdit::editingFinished,
            this, &CertificateLineEdit::updateKey);
    connect(this, &QLineEdit::textChanged,
            this, &CertificateLineEdit::editChanged);
    connect(mLineAction, &QAction::triggered,
            this, &CertificateLineEdit::dialogRequested);
    connect(this, &QLineEdit::editingFinished, this,
            &CertificateLineEdit::checkLocate);
    updateKey();

    /* Take ownership of the model to prevent double deletion when the
     * filter models are deleted */
    model->setParent(parent ? parent : this);
}

void CertificateLineEdit::editChanged()
{
    updateKey();
    if (!mEditStarted) {
        Q_EMIT editingStarted();
        mEditStarted = true;
    }
    mEditFinished = false;
}

void CertificateLineEdit::checkLocate()
{
    if (!key().isNull()) {
        // Already have a key
        return;
    }

    // Only check once per mailbox
    const auto mailText = text();
    if (s_lookedUpKeys.contains(mailText)) {
        return;
    }
    s_lookedUpKeys << mailText;
    qCDebug(KLEOPATRA_LOG) << "Lookup job for" << mailText;
    QGpgME::KeyForMailboxJob *job = QGpgME::openpgp()->keyForMailboxJob();
    job->start(mailText);
}

void CertificateLineEdit::updateKey()
{
    const auto mailText = text();
    auto newKey = Key();
    if (mailText.isEmpty()) {
        mLineAction->setIcon(QIcon::fromTheme(QStringLiteral("resource-group-new")));
        mLineAction->setToolTip(i18n("Open selection dialog."));
    } else {
        mFilterModel->setFilterFixedString(mailText);
        if (mFilterModel->rowCount() > 1) {
            if (mEditFinished) {
                mLineAction->setIcon(QIcon::fromTheme(QStringLiteral("question")).pixmap(KIconLoader::SizeSmallMedium));
                mLineAction->setToolTip(i18n("Multiple certificates"));
            }
        } else if (mFilterModel->rowCount() == 1) {
            newKey = mFilterModel->data(mFilterModel->index(0, 0), KeyListModelInterface::KeyRole).value<Key>();
            mLineAction->setToolTip(Formatting::validity(newKey.userID(0)) +
                                    QStringLiteral("<br/>Click here for details."));
            /* FIXME: This needs to be solved by a multiple UID supporting model */
            mLineAction->setIcon(Formatting::iconForUid(newKey.userID(0)));
        } else {
            mLineAction->setIcon(QIcon::fromTheme(QStringLiteral("emblem-error")));
            mLineAction->setToolTip(i18n("No matching certificates found.<br/>Click here to import a certificate."));
        }
    }
    mKey = newKey;

    if (mKey.isNull()) {
        setToolTip(QString());
    } else {
        setToolTip(Formatting::toolTip(newKey, Formatting::ToolTipOption::AllOptions));
    }

    Q_EMIT keyChanged();

    if (mailText.isEmpty()) {
        Q_EMIT wantsRemoval(this);
    }
}

Key CertificateLineEdit::key() const
{
    if (isEnabled()) {
        return mKey;
    } else {
        return Key();
    }
}

void CertificateLineEdit::dialogRequested()
{
    if (!mKey.isNull()) {
        auto cmd = new Commands::DetailsCommand(mKey, nullptr);
        cmd->start();
        return;
    }

    CertificateSelectionDialog *const dlg = new CertificateSelectionDialog(this);

    dlg->setKeyFilter(mFilter);

    if (dlg->exec()) {
        const std::vector<Key> keys = dlg->selectedCertificates();
        if (!keys.size()) {
            return;
        }
        for (unsigned int i = 0; i < keys.size(); i++) {
            if (!i) {
                setKey(keys[i]);
            } else {
                Q_EMIT addRequested(keys[i]);
            }
        }
    }
    delete dlg;
    updateKey();
}

void CertificateLineEdit::setKey(const Key &k)
{
    QSignalBlocker blocky(this);
    qCDebug(KLEOPATRA_LOG) << "Setting Key. " << Formatting::summaryLine(k);
    setText(Formatting::summaryLine(k));
    updateKey();
}

bool CertificateLineEdit::isEmpty() const
{
    return text().isEmpty();
}

void CertificateLineEdit::setKeyFilter(const std::shared_ptr<KeyFilter> &filter)
{
    mFilter = filter;
    mFilterModel->setKeyFilter(filter);
}

#include "certificatelineedit.moc"
