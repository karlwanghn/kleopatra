#ifndef VIEW_SMARTCARDWIDGET_H
#define VIEW_SMARTCARDWIDGET_H
/*  view/smartcardwidget.h

    This file is part of Kleopatra, the KDE keymanager
    SPDX-FileCopyrightText: 2017 Bundesamt für Sicherheit in der Informationstechnik
    SPDX-FileContributor: Intevation GmbH

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QWidget>
#include <memory>

namespace Kleo
{
/* SmartCardWidget a generic widget to interact with smartcards */
class SmartCardWidget: public QWidget
{
    Q_OBJECT
public:
    explicit SmartCardWidget(QWidget *parent = nullptr);

public Q_SLOTS:
    void reload();

Q_SIGNALS:
    void backRequested();

private:
    class Private;
    std::shared_ptr<Private> d;
};

} // namespace Kleo
#endif // VIEW_SMARTCARDWIDGET_H
