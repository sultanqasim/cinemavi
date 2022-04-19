#include "cmrawinfowidget.h"
#include <QGridLayout>
#include <QDateTime>
#include <cstring>

CMRawInfoWidget::CMRawInfoWidget(QWidget *parent)
    : QGroupBox{parent}
{
    this->setTitle(tr("Capture Info"));
    QGridLayout *gl = new QGridLayout(this);

    QLabel *labShutter = new QLabel(tr("Shutter"), this);
    this->shutterLabel = new QLabel(this);
    gl->addWidget(labShutter, 0, 0);
    gl->addWidget(shutterLabel, 0, 1);

    QLabel *labGain = new QLabel(tr("Gain"), this);
    this->gainLabel = new QLabel(this);
    gl->addWidget(labGain, 0, 2);
    gl->addWidget(gainLabel, 0, 3);

    QLabel *labDate = new QLabel(tr("Date"), this);
    this->dateLabel = new QLabel(this);
    gl->addWidget(labDate, 1, 0);
    gl->addWidget(dateLabel, 1, 1);

    QLabel *labTime = new QLabel(tr("Time"), this);
    this->timeLabel = new QLabel(this);
    gl->addWidget(labTime, 1, 2);
    gl->addWidget(timeLabel, 1, 3);

    QLabel *labCamera = new QLabel(tr("Camera"), this);
    this->cameraLabel = new QLabel(this);
    gl->addWidget(labCamera, 2, 0);
    gl->addWidget(cameraLabel, 2, 1, 1, 3);

    gl->setColumnStretch(1, 1);
    gl->setColumnStretch(3, 1);
}

void CMRawInfoWidget::setRawHeader(CMRawHeader &cmrh)
{
    if (cmrh.cinfo.shutter_us > 1000)
        shutterLabel->setText(QString::asprintf("%.1f ms", cmrh.cinfo.shutter_us * 0.001));
    else
        shutterLabel->setText(QString::asprintf("%.1f us", cmrh.cinfo.shutter_us));

    gainLabel->setText(QString::asprintf("%.1f dB", cmrh.cinfo.gain_dB));

    char camMake[sizeof(cmrh.camera_make)] = {};
    char camModel[sizeof(cmrh.camera_model)] = {};
    memcpy(camMake, cmrh.camera_make, sizeof(cmrh.camera_make) - 1);
    memcpy(camModel, cmrh.camera_model, sizeof(cmrh.camera_model) - 1);
    cameraLabel->setText(QString::asprintf("%s %s", camMake, camModel));

    QDateTime shotTime = QDateTime::fromSecsSinceEpoch(cmrh.cinfo.ts_epoch);
    dateLabel->setText(shotTime.toString("yyyy-MM-dd"));
    timeLabel->setText(shotTime.toString("h:mm:ss AP"));
}
