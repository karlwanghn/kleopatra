/* -*- mode: c++; c-basic-offset:4 -*-
    importcertificatefromdatacommand.h

    This file is part of Kleopatra, the KDE keymanager
    SPDX-FileCopyrightText: 2018 Intevation GmbH

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef __KLEOPATRA_IMPORTCERTIFICATEFROMDATACOMMAND_H__
#define __KLEOPATRA_IMPORTCERTIFICATEFROMDATACOMMAND_H__

#include "importcertificatescommand.h"

#include <gpgme++/global.h>


namespace Kleo
{

class ImportCertificateFromDataCommand : public ImportCertificatesCommand
{
    Q_OBJECT
public:
    explicit ImportCertificateFromDataCommand(const QByteArray &data,
                                              GpgME::Protocol proto);
    ~ImportCertificateFromDataCommand() override;

private:
    void doStart() override;

private:
    class Private;
    inline Private *d_func();
    inline const Private *d_func() const;
};
}

#endif // __KLEOPATRA_IMPORTCERTIFICATEFROMDATACOMMAND_H__

