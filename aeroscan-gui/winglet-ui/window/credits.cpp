#include "credits.h"
#include "ui_credits.h"
#include <QKeyEvent>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

#include "wingletgui.h"
#include "winglet-ui/theme.h"

namespace WingletUI {

Credits::Credits(QWidget *parent) :
    QWidget(parent), ui(new Ui::Credits),
    avLogoLabel(new QLabel(this))
{
    ui->setupUi(this);
    ui->title_label->setForegroundRole(QPalette::Text);
    ui->title_label->setFont(QFont(activeTheme->titleFont, 30));

    activeTheme->renderBgAvLogo(avLogoLabel);

    // Create animation group
    animationGroup = new QParallelAnimationGroup(this);
    connect(animationGroup, SIGNAL(finished()), this, SLOT(animationGroupFinished()));

    // Define graphics effects to fade in/out screen
    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(ui->label);
    effect->setOpacity(0.0);
    ui->label->setGraphicsEffect(effect);

    effect = new QGraphicsOpacityEffect(ui->title_label);
    effect->setOpacity(0.0);
    ui->title_label->setGraphicsEffect(effect);
}

Credits::~Credits()
{
    delete ui;
}

void Credits::showEvent(QShowEvent *ev)
{
    (void) ev;
    setLeds();

    // Start the show animation
    animationGroup->clear();
    animationIsHiding = false;

    if (activeTheme->animationDuration) {
        QPropertyAnimation* anim = new QPropertyAnimation(ui->label->graphicsEffect(), "opacity");
        anim->setDuration(activeTheme->animationDuration);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        animationGroup->addAnimation(anim);

        anim = new QPropertyAnimation(ui->title_label->graphicsEffect(), "opacity");
        anim->setDuration(activeTheme->animationDuration);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        animationGroup->addAnimation(anim);

        animationGroup->start();
    }
    else {
        ui->label->graphicsEffect()->setProperty("opacity", 1.0);
        ui->title_label->graphicsEffect()->setProperty("opacity", 1.0);
    }
}

void Credits::hideEvent(QHideEvent *ev)
{
    (void) ev;
}

void Credits::animationGroupFinished()
{
    if (animationIsHiding) {
        WingletGUI::inst->removeWidgetOnTop(this);
    }
}

void Credits::setLeds()
{
}

void Credits::beginHide()
{
    // Start the hide animation
    animationGroup->clear();
    animationIsHiding = true;

    if (!activeTheme->animationDuration)
    {
        animationGroupFinished();
        return;
    }

    QPropertyAnimation* anim = new QPropertyAnimation(ui->label->graphicsEffect(), "opacity");
    anim->setDuration(activeTheme->animationDuration);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);
    animationGroup->addAnimation(anim);

    anim = new QPropertyAnimation(ui->title_label->graphicsEffect(), "opacity");
    anim->setDuration(activeTheme->animationDuration);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);
    animationGroup->addAnimation(anim);

    animationGroup->start();
}

void Credits::keyPressEvent(QKeyEvent *ev)
{
    if (ev->key() == Qt::Key::Key_B) {
        beginHide();
    }
}

} // namespace WingletUI
