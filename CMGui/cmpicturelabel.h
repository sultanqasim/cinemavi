#ifndef CMPICTURELABEL_H
#define CMPICTURELABEL_H

#include <QWidget>
#include <QLabel>
#include <QImage>
#include <QColorTransform>

class CMPictureLabel : public QWidget
{
    Q_OBJECT

public:
    explicit CMPictureLabel(QWidget *parent = nullptr);
    ~CMPictureLabel();

public slots:
    void setImage(const QImage &img);

signals:
    void picturePressed(uint16_t posX, uint16_t posY);

private:
    QLabel *imgLabel;
    QPixmap imgMap;
    QColorTransform colourTransform;
    bool colourTransformValid;
    void regenPixmap(const QSize &size);
    QString getDisplayProfileURL();
    void loadDisplayColourTransform();

protected:
    void resizeEvent(QResizeEvent *event);
    void mousePressEvent(QMouseEvent *event);
};

#endif // CMPICTURELABEL_H
