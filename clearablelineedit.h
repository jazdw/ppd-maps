#ifndef CLEARABLELINEEDIT_H
#define CLEARABLELINEEDIT_H

#include <QLineEdit>
#include <QToolButton>
#include <QStyle>
#include <QPixmap>

class clearableLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit clearableLineEdit(QWidget *parent = 0);
protected:
    void resizeEvent(QResizeEvent *);
private slots:
    void updateCloseButton(const QString &text);
private:
    QToolButton *clearButton;
};

#endif // CLEARABLELINEEDIT_H
