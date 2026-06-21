#include "addserverdialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QVBoxLayout>

namespace amule {

AddServerDialog::AddServerDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Add server"));

    nameEdit_ = new QLineEdit;
    nameEdit_->setPlaceholderText(tr("optional name"));
    addressEdit_ = new QLineEdit;
    addressEdit_->setPlaceholderText(tr("host:port"));

    auto* form = new QFormLayout;
    form->addRow(tr("Name:"), nameEdit_);
    form->addRow(tr("Address:"), addressEdit_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

QString AddServerDialog::serverName() const {
    return nameEdit_->text().trimmed();
}

QString AddServerDialog::address() const {
    return addressEdit_->text().trimmed();
}

} // namespace amule
