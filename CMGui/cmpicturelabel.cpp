#include "cmpicturelabel.h"

#include <QResizeEvent>
#include <QImage>

CMPictureLabel::CMPictureLabel(QWidget *parent) :
    QWidget(parent)
{
    imgLabel = new QLabel(this);
    imgLabel->setAlignment(Qt::AlignCenter);
    imgLabel->setPixmap(imgMap);
}

CMPictureLabel::~CMPictureLabel() {}

void CMPictureLabel::resizeEvent(QResizeEvent *event)
{
    this->regenPixmap(event->size());
    imgLabel->resize(event->size());
    QWidget::resizeEvent(event);
}

void CMPictureLabel::setImage(const uint8_t *img_rgb8, uint16_t width, uint16_t height)
{
    QImage img(img_rgb8, width, height, width*3, QImage::Format_RGB888);

    this->imgMap.convertFromImage(img);
    this->regenPixmap(this->imgLabel->size());
}

void CMPictureLabel::setPixmap(const QPixmap &pm) {
    this->imgMap = pm;
    this->regenPixmap(this->imgLabel->size());
}

void CMPictureLabel::regenPixmap(const QSize &size) {
    if (!this->imgMap.isNull()) {
        QPixmap pm = this->imgMap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        this->imgLabel->setPixmap(pm);
    }
}
