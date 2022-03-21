#include "cmcameracontrols.h"

#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QStandardPaths>
#include <QFileDialog>

CMCameraControls::CMCameraControls(QWidget *parent)
    : QWidget{parent}
{
    QVBoxLayout *cvl = new QVBoxLayout(this);

    QGroupBox *exposureGroup = new QGroupBox(tr("Exposure"), this);
    QGroupBox *captureGroup = new QGroupBox(tr("Capture"), this);
    this->shootButton = new QPushButton(tr("Shoot"), this);

    cvl->addWidget(exposureGroup);
    cvl->addWidget(captureGroup);
    cvl->addWidget(shootButton);

    QGridLayout *egl = new QGridLayout(exposureGroup);
    egl->setColumnMinimumWidth(0, 60);
    egl->setColumnStretch(1, 1);
    QLabel *modeLabel = new QLabel(tr("Mode"), exposureGroup);
    this->expModeSelector = new QComboBox(exposureGroup);
    this->expModeSelector->addItem(tr("Manual"), CMEXP_MANUAL);
    this->expModeSelector->addItem(tr("Auto"), CMEXP_AUTO);
    this->expModeSelector->addItem(tr("Shutter Priority"), CMEXP_SHUTTER_PRIORITY);
    this->expModeSelector->addItem(tr("Gain Priority"), CMEXP_GAIN_PRIORITY);
    egl->addWidget(modeLabel, 0, 0);
    egl->addWidget(expModeSelector, 0, 1);
    QLabel *shutterLabel = new QLabel(tr("Shutter (ms)"), exposureGroup);
    this->shutterSlider = new CMNumberSlider(exposureGroup);
    this->shutterSlider->setMinMax(0.5, 250, true); // TODO: get values from camera
    egl->addWidget(shutterLabel, 1, 0);
    egl->addWidget(shutterSlider, 1, 1);
    QLabel *gainLabel = new QLabel(tr("Gain (dB)"), exposureGroup);
    this->gainSlider = new CMNumberSlider(exposureGroup);
    this->gainSlider->setMinMax(1, 48); // TODO: get values from camera
    egl->addWidget(gainLabel, 2, 0);
    egl->addWidget(gainSlider, 2, 1);

    QGridLayout *cgl = new QGridLayout(captureGroup);
    cgl->setColumnMinimumWidth(0, 60);
    cgl->setColumnStretch(1, 1);
    QLabel *formatLabel = new QLabel(tr("Format"), captureGroup);
    this->formatSelector = new QComboBox(captureGroup);
    this->formatSelector->addItem(tr("CMRAW"), CMCAP_CMRAW);
    this->formatSelector->addItem(tr("DNG"), CMCAP_DNG);
    this->formatSelector->addItem(tr("TIFF"), CMCAP_TIFF);
    cgl->addWidget(formatLabel, 0, 0);
    cgl->addWidget(formatSelector, 0, 1);
    QLabel *pathLabel = new QLabel(tr("Path"), captureGroup);
    QHBoxLayout *pathLineLayout = new QHBoxLayout();
    cgl->addWidget(pathLabel, 1, 0);
    cgl->addLayout(pathLineLayout, 1, 1);
    this->pathLine = new QLineEdit(captureGroup);
    this->pathLine->setText(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/Cinemavi");
    this->pathLine->setReadOnly(true);
    QPushButton *pathEditButton = new QPushButton(tr("Edit"), captureGroup);
    pathLineLayout->addWidget(pathLine, 1);
    pathLineLayout->addWidget(pathEditButton);

    connect(this->shootButton, &QPushButton::clicked, this, &CMCameraControls::onShoot);
    connect(this->shutterSlider, &CMNumberSlider::valueChanged, this, &CMCameraControls::onShutterChanged);
    connect(this->gainSlider, &CMNumberSlider::valueChanged, this, &CMCameraControls::onGainChanged);
    connect(this->expModeSelector, &QComboBox::currentIndexChanged, this, &CMCameraControls::onExpModeChanged);
    connect(pathEditButton, &QPushButton::clicked, this, &CMCameraControls::onChoosePath);
}

void CMCameraControls::setShootEnabled(bool e)
{
    this->shootButton->setEnabled(e);
}

QString CMCameraControls::saveDir()
{
    return this->pathLine->text();
}

void CMCameraControls::onShoot()
{
    emit shootClicked();
}

void CMCameraControls::onExpModeChanged(int index)
{
    CMExposureMode expMode = (CMExposureMode)index;
    emit exposureChanged(expMode, this->shutterSlider->value() * 1000, this->gainSlider->value());
    switch (expMode) {
    case CMEXP_MANUAL:
        this->shutterSlider->setReadOnly(false);
        this->gainSlider->setReadOnly(false);
        break;
    case CMEXP_AUTO:
        this->shutterSlider->setReadOnly(true);
        this->gainSlider->setReadOnly(true);
        break;
    case CMEXP_SHUTTER_PRIORITY:
        this->shutterSlider->setReadOnly(false);
        this->gainSlider->setReadOnly(true);
        break;
    case CMEXP_GAIN_PRIORITY:
        this->shutterSlider->setReadOnly(true);
        this->gainSlider->setReadOnly(false);
        break;
    }
}

// only emit signal if slider change was not automated
void CMCameraControls::onShutterChanged(double val)
{
    CMExposureMode expMode = (CMExposureMode)this->expModeSelector->currentIndex();
    if (expMode == CMEXP_MANUAL || expMode == CMEXP_SHUTTER_PRIORITY)
        emit exposureChanged(expMode, val * 1000, this->gainSlider->value());
}

void CMCameraControls::onGainChanged(double val)
{
    CMExposureMode expMode = (CMExposureMode)this->expModeSelector->currentIndex();
    if (expMode == CMEXP_MANUAL || expMode == CMEXP_GAIN_PRIORITY)
        emit exposureChanged(expMode, this->shutterSlider->value() * 1000, val);
}

void CMCameraControls::onChoosePath()
{
    QString dirName = QFileDialog::getExistingDirectory(this, tr("Select Directory"),
            this->pathLine->text(), QFileDialog::ShowDirsOnly);
    if (dirName.isNull())
        return;
    this->pathLine->setText(dirName);
}

void CMCameraControls::setShutter(double shutter_us)
{
    this->shutterSlider->setValue(shutter_us * 0.001);
}

void CMCameraControls::setGain(double gain_dB)
{
    this->gainSlider->setValue(gain_dB);
}

double CMCameraControls::getShutter()
{
    return this->shutterSlider->value() * 1000;
}

double CMCameraControls::getGain()
{
    return this->gainSlider->value();
}

CMCaptureFormat CMCameraControls::getCaptureFormat()
{
    return (CMCaptureFormat)this->formatSelector->currentIndex();
}

CMExposureMode CMCameraControls::getExposureMode()
{
    return (CMExposureMode)this->expModeSelector->currentIndex();
}
