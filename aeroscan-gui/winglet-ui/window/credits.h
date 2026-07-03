#ifndef CREDITS_H
#define CREDITS_H

#include <QWidget>
#include <QLabel>
#include <QParallelAnimationGroup>
#include <unistd.h>

namespace Ui {
class Credits;
}

namespace WingletUI {

class Credits : public QWidget
{
    Q_OBJECT

public:
    explicit Credits(QWidget *parent = nullptr);
    ~Credits();

protected:
    void showEvent(QShowEvent *ev) override;
    void hideEvent(QHideEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;

private slots:
    void animationGroupFinished();

private:
    void setLeds();
    void beginHide();

    Ui::Credits *ui;
    QLabel *avLogoLabel;
    QParallelAnimationGroup *animationGroup;
    bool animationIsHiding;
};

} // namespace WingletUI

#endif // CREDITS_H
