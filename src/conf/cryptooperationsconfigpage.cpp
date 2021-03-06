/* -*- mode: c++; c-basic-offset:4 -*-
    conf/cryptooperationsconfigpage.cpp

    This file is part of Kleopatra, the KDE keymanager
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <config-kleopatra.h>

#include "cryptooperationsconfigpage.h"

#include "cryptooperationsconfigwidget.h"

#include <QVBoxLayout>

using namespace Kleo;
using namespace Kleo::Config;

CryptoOperationsConfigurationPage::CryptoOperationsConfigurationPage(QWidget *parent, const QVariantList &args)
    : KCModule(parent, args)
{
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    mWidget = new CryptoOperationsConfigWidget(this);
    lay->addWidget(mWidget);
    connect(mWidget, &CryptoOperationsConfigWidget::changed, this, &Kleo::Config::CryptoOperationsConfigurationPage::markAsChanged);
    load();
}

void CryptoOperationsConfigurationPage::load()
{
    mWidget->load();
}

void CryptoOperationsConfigurationPage::save()
{
    mWidget->save();

}

void CryptoOperationsConfigurationPage::defaults()
{
    mWidget->defaults();
}

extern "C"
{
    Q_DECL_EXPORT KCModule *create_kleopatra_config_cryptooperations(QWidget *parent = nullptr, const QVariantList &args = QVariantList())
    {
        CryptoOperationsConfigurationPage *page =
            new CryptoOperationsConfigurationPage(parent, args);
        page->setObjectName(QStringLiteral("kleopatra_config_cryptooperations"));
        return page;
    }
}

