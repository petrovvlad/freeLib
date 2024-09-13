#include "passwordlineedit.h"
#include "options.h"

#include "utilites.h"


PasswordLineEdit::PasswordLineEdit(QWidget *parent)
    :QLineEdit(parent)
{
}

void PasswordLineEdit::setPassword(const QByteArray &baSalt, const QByteArray &baHash)
{
    baSalt_ = baSalt;
    baHash_ = baHash;
    if(!baHash.isEmpty() && baHash != Options::passwordToHash(QStringLiteral(""), baSalt))
    {
        setText(u" ********** "_s);
    }
}

QByteArray PasswordLineEdit::getPasswordHash()
{
    if(!sText_.isEmpty() && sText_ != u" ********** "_s){
        QByteArray newHash = Options::passwordToHash(sText_, baSalt_);
        if(baHash_ != newHash) // Генерируем новую соль если пароль изменился
        {
            baSalt_ = Options::generateSalt();
            baHash_ =  Options::passwordToHash(sText_, baSalt_);
        }
    }
    return baHash_;
}

QByteArray PasswordLineEdit::getPasswordSalt()
{
    return baSalt_;
}

void PasswordLineEdit::focusInEvent(QFocusEvent *event)
{
    if(!sText_.isEmpty() && sText_ == u" ********** "_s){
        setText(u""_s);
    }else
        setText(sText_);

    // Вызов базовой реализации обработчика события
    QLineEdit::focusInEvent(event);
}

void PasswordLineEdit::focusOutEvent(QFocusEvent *event)
{
    sText_ = text();
    if(!sText_.isEmpty() || !baHash_.isEmpty()){
        setText(u" ********** "_s);
    }

    // Вызов базовой реализации обработчика события
    QLineEdit::focusOutEvent(event);

}
