#include "simplemediaplayer.h"
#include "wingletgui.h"
#include <QKeyEvent>
#include <QWheelEvent>
#include <QMovie>
#include <QTimer>

#define MAXNUMMOVIES 5
#define MAXNUMMOVIESCUSTOM 6
#define CUSTOM_MEDIA_URL "/mnt/sd/custom_media.gif"

namespace WingletUI {

SimpleMediaPlayer::SimpleMediaPlayer(QWidget *parent)
    : QWidget{parent}
{
    label = new QLabel(this);
    label->setGeometry(0, 0, 480, 480);

    QTimer* tir = new QTimer(this);
    connect(tir, SIGNAL(timeout()), this, SLOT(shimmerIncrement()));
    tir->start(100);

    startMovie();
}

SimpleMediaPlayer::~SimpleMediaPlayer()
{
    if (movie)
        delete movie;
    delete label;
}

void SimpleMediaPlayer::hideEvent(QHideEvent *ev)
{
    (void) ev;
}

void SimpleMediaPlayer::wheelEvent(QWheelEvent *ev) {

    int max_num_movies = MAXNUMMOVIES;
    if (QFile::exists(CUSTOM_MEDIA_URL)) {
        max_num_movies = MAXNUMMOVIESCUSTOM;
    }

    if(ev->angleDelta().y() > 0){
        if(this->movIndx < max_num_movies){
            movIndx = movIndx+1;
            startMovie();
        }
    } else {
        if(this->movIndx > 0){
            movIndx = movIndx-1;
            startMovie();
        }
    }
}

void SimpleMediaPlayer::keyPressEvent(QKeyEvent *ev) {
    int max_num_movies = MAXNUMMOVIES;
    if (QFile::exists(CUSTOM_MEDIA_URL)) {
        max_num_movies = MAXNUMMOVIESCUSTOM;
    }

    switch (ev->key()) {
    case Qt::Key_Right:
        if(this->movIndx < max_num_movies){
            movIndx = movIndx+1;
            startMovie();
        }
        break;
    case Qt::Key_Left:
        if(this->movIndx > 0){
            movIndx = movIndx-1;
            startMovie();
        }
        break;
    default:
        ev->ignore();
    }
}

void SimpleMediaPlayer::startMovie()
{
    QMovie *prevMovie = movie;
    switch (movIndx)
    {
    case 0:
    default:
        movie = new QMovie(":/videos/SpaceDebrie280.gif");
        break;
    case 1:
        movie = new QMovie(":/videos/Jupiter.gif");
        break;
    case 2:
        movie = new QMovie(":/videos/Earth.gif");
        break;
    case 3:
        movie = new QMovie(":/videos/volleyball.gif");
        break;
    case 4:
        movie = new QMovie(":/videos/DarkStar.gif");
        break;
    case 5:
        movie = new QMovie(":/videos/DBZ.gif");;
        break;
    case 6:
        movie = new QMovie(CUSTOM_MEDIA_URL);
        break;
    }

    movie->setScaledSize(QSize(480, 480));
    label->setMovie(movie);
    movie->start();

    if (prevMovie)
        delete prevMovie;

}

void SimpleMediaPlayer::shimmerIncrement(){
    // this->shimmer_angle += 10;
    // if(this->shimmer_angle >= 360){
    //     this->shimmer_angle = this->shimmer_angle % 360;
    // }
    // rgbled_show_shimmer(this->shimmer_angle);
        QImage currentFrame = movie->currentImage(); // Get the current frame as a QImage

        if (!currentFrame.isNull()) {
            /// led mapping to x,y
            /// 0 278,478
            /// 1 203,478
            /// 2 132,454
            /// 3 70,410
            /// 4 27,349
            /// 5 3,278
            /// 6 3,203
            /// 7 27,131
            /// 8 70,70
            /// 9 132,27
            /// 10 203,3
            /// 11 278,3
            /// 12 349,27
            /// 13 410,70
            /// 14 454,131
            /// 15 477,203
            /// 16 477,278
            /// 17 454,349
            /// 18 410,410
            /// 19 349,454
            ///
            QColor pixelColor0 = currentFrame.pixelColor(278,478);
            QColor pixelColor1 = currentFrame.pixelColor(203,478);
            QColor pixelColor2 = currentFrame.pixelColor(132,454);
            QColor pixelColor3 = currentFrame.pixelColor(70,410);
            QColor pixelColor4 = currentFrame.pixelColor(27,349);
            QColor pixelColor5 = currentFrame.pixelColor(3,278);
            QColor pixelColor6 = currentFrame.pixelColor(3,203);
            QColor pixelColor7 = currentFrame.pixelColor(27,131);
            QColor pixelColor8 = currentFrame.pixelColor(70,70);
            QColor pixelColor9 = currentFrame.pixelColor(132,27);
            QColor pixelColor10 = currentFrame.pixelColor(203,3);
            QColor pixelColor11 = currentFrame.pixelColor(278,3);
            QColor pixelColor12 = currentFrame.pixelColor(349,27);
            QColor pixelColor13 = currentFrame.pixelColor(410,70);
            QColor pixelColor14 = currentFrame.pixelColor(454,131);
            QColor pixelColor15 = currentFrame.pixelColor(477,203);
            QColor pixelColor16 = currentFrame.pixelColor(477,278);
            QColor pixelColor17 = currentFrame.pixelColor(454,349);
            QColor pixelColor18 = currentFrame.pixelColor(410,410);
            QColor pixelColor19 = currentFrame.pixelColor(349,454);


        }

}

} // namespace WingletUI
