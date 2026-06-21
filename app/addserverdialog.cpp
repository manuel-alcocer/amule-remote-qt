#include "addserverdialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QVBoxLayout>

namespace amule {

AddServerDialog::AddServerDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("Add server"));

    nameEdit_ = new QLineEdit;
    nameEdit_->setPlaceholderText(QStringLiteral("optional name"));
    addressEdit_ = new QLineEdit;
    addressEdit_->setPlaceholderText(QStringLiteral("host:port"));

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Name:"), nameEdit_);
    form->addRow(QStringLiteral("Address:"), addressEdit_);

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
