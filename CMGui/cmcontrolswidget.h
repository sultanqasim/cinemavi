#ifndef CMCONTROLSWIDGET_H
#define CMCONTROLSWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include "cmnumberslider.h"
#include "../pipeline.h"

class CMControlsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CMControlsWidget(QWidget *parent = nullptr);
    void getParams(ImagePipelineParams *params);
    void setWhiteBalance(double temp_K, double tint);
    void setShotWhiteBalance(double temp_K = 5000, double tint = 0);
    bool spotWhiteChecked();

signals:
    void paramsChanged();
    void autoWhiteBalanceTriggered(CMAutoWhiteMode mode);

public slots:
    void onDebayerModeChanged(int index);
    void onNRModeChanged(int index);
    void onLUTModeChanged(int index);
    void onSliderChanged(double val);
    void onBrightsWhiteBalance();
    void onRobustWhiteBalance();
    void onReset();

private:
    CMNumberSlider *expSlider;
    CMNumberSlider *warmthSlider;
    CMNumberSlider *tintSlider;
    CMNumberSlider *hueSlider;
    CMNumberSlider *satSlider;
    QComboBox *debayerModeSelector;
    QComboBox *nrModeSelector;
    CMNumberSlider *lumaSlider;
    CMNumberSlider *chromaSlider;
    QComboBox *tmModeSelector;
    CMNumberSlider *gammaSlider;
    CMNumberSlider *shadowSlider;
    CMNumberSlider *blackSlider;
    QPushButton *spotWhiteButton;
    double shotTempK = 5000;
    double shotTint = 0;
};

#endif // CMCONTROLSWIDGET_H
