#ifndef CMCAMERACONTROLS_H
#define CMCAMERACONTROLS_H

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include "cmnumberslider.h"
#include "cmautoexposure.h"

typedef enum {
    CMCAP_CMRAW,
    CMCAP_DNG,
    CMCAP_TIFF,
    CMCAP_JPEG
} CMCaptureFormat;

class CMCameraControls : public QWidget
{
    Q_OBJECT
public:
    explicit CMCameraControls(QWidget *parent = nullptr);
    QString saveDir();
    CMCaptureFormat captureFormat();
    void setShootEnabled(bool e);
    void setShutter(double shutter_us);
    void setGain(double gain_dB);
    CMExposureMode exposureMode();
    double getShutter();
    double getGain();

signals:
    // not emitted in auto modes for automatically controlled parameter changes
    void exposureChanged(CMExposureMode mode, double shutter_us, double gain_dB);
    void shootClicked();

public slots:
    void onShoot();
    void onExpModeChanged(int index);
    void onChoosePath();
    void onShutterChanged(double val);
    void onGainChanged(double val);

private:
    QComboBox *expModeSelector;
    CMNumberSlider *shutterSlider;
    CMNumberSlider *gainSlider;
    QComboBox *formatSelector;
    QLineEdit *pathLine;
    QPushButton *shootButton;
};

#endif // CMCAMERACONTROLS_H
