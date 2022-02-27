#include "cmpicturelabel.h"

#include <QResizeEvent>
#include <QImage>

CMPictureLabel::CMPictureLabel(QWidget *parent) :
    QWidget(parent)
{
    imgLabel = new QLabel(this);
    imgLabel->setAlignment(Qt::AlignCenter);
    imgMap = QPixmap("/Users/sultan/Documents/cinemavi/test_cubic.tiff");
    imgLabel->setPixmap(imgMap);
}

CMPictureLabel::~CMPictureLabel() {}

void CMPictureLabel::resizeEvent(QResizeEvent *event)
{
    if (!imgMap.isNull()) {
        QPixmap px = imgMap.scaled(event->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        imgLabel->setPixmap(px);
    }
    imgLabel->resize(event->size());
    QWidget::resizeEvent(event);
}

void CMPictureLabel::setImage(const uint8_t *img_rgb8, uint16_t width, uint16_t height)
{
    QImage img(img_rgb8, width, height, width, QImage::Format_RGB888);

    this->imgMap.convertFromImage(img);
    this->imgLabel->setPixmap(this->imgMap.scaled(
            this->imgLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
