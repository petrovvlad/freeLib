#ifndef PASSWORDLINEEDIT_H
#define PASSWORDLINEEDIT_H

#include <QLineEdit>

class PasswordLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    PasswordLineEdit(QWidget *parent = nullptr);

    void setPassword(const QByteArray &baSalt, const QByteArray &baHash);
    QByteArray getPasswordHash(/*const QByteArray &baSalt*/);
    QByteArray getPasswordSalt();

protected:
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    QByteArray baSalt_;
    QByteArray baHash_;
    QString sText_;
};

#endif // PASSWORDLINEEDIT_H
