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

void CMPictureLabel::mousePressEvent(QMouseEvent *event)
{
    QPoint labelClickPos = this->imgLabel->mapFromParent(event->pos());

    double labWidth = this->imgLabel->width();
    double labHeight = this->imgLabel->height();
    double imgWidth = this->imgMap.width();
    double imgHeight = this->imgMap.height();

    if (imgWidth > 0) {
        double imgShapeFactor = imgHeight / imgWidth;
        double labelShapeFactor = labHeight / labWidth;

        double originX = 0;
        double originY = 0;
        double imgPixPerScreen;
        if (labelShapeFactor > imgShapeFactor) {
            // label is taller than image
            double labPmHeight = this->imgLabel->pixmap().height();
            originY = (labHeight - labPmHeight) * 0.5;
            imgPixPerScreen = imgWidth / labWidth;
        } else {
            // label is wider than image
            double labPmWidth = this->imgLabel->pixmap().width();
            originX = (labWidth - labPmWidth) * 0.5;
            imgPixPerScreen = imgHeight / labHeight;
        }

        double imgX = imgPixPerScreen * (labelClickPos.x() - originX);
        double imgY = imgPixPerScreen * (labelClickPos.y() - originY);

        if (imgX >= 0 && imgX < imgWidth && imgY >= 0 && imgY < imgHeight)
            emit picturePressed(imgX, imgY);
    }

    QWidget::mousePressEvent(event);
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
    } else {
        this->imgLabel->setPixmap(this->imgMap);
    }
}
