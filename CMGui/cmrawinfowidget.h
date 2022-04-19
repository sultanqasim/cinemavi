#ifndef CMRAWINFOWIDGET_H
#define CMRAWINFOWIDGET_H

#include <QGroupBox>
#include <QLabel>
#include "../cmraw.h"

class CMRawInfoWidget : public QGroupBox
{
    Q_OBJECT
public:
    explicit CMRawInfoWidget(QWidget *parent = nullptr);
    void setRawHeader(CMRawHeader &cmrh);

signals:

private:
    QLabel *shutterLabel;
    QLabel *gainLabel;
    QLabel *dateLabel;
    QLabel *timeLabel;
    QLabel *cameraLabel;
};

#endif // CMRAWINFOWIDGET_H
