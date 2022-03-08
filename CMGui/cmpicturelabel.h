#ifndef CMPICTURELABEL_H
#define CMPICTURELABEL_H

#include <QWidget>
#include <QLabel>
#include <QPixmap>

class CMPictureLabel : public QWidget
{
    Q_OBJECT

public:
    explicit CMPictureLabel(QWidget *parent = nullptr);
    ~CMPictureLabel();
    void setImage(const uint8_t *img_rgb8, uint16_t width, uint16_t height);

public slots:
    void setPixmap(const QPixmap &pm);

signals:
    void picturePressed(uint16_t posX, uint16_t posY);

private:
    QLabel *imgLabel;
    QPixmap imgMap;
    void regenPixmap(const QSize &size);

protected:
    void resizeEvent(QResizeEvent *event);
    void mousePressEvent(QMouseEvent *event);
};

#endif // CMPICTURELABEL_H
