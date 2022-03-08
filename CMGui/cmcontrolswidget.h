#ifndef CMCONTROLSWIDGET_H
#define CMCONTROLSWIDGET_H

#include <QWidget>
#include <QComboBox>
#include "cmnumberslider.h"
#include "../pipeline.h"

class CMControlsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CMControlsWidget(QWidget *parent = nullptr);
    void getParams(ImagePipelineParams *params);
    void setWhiteBalance(double temp_K, double tint);

signals:
    void paramsChanged();
    void autoWhiteBalanceTriggered(CMAutoWhiteMode mode);

public slots:
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
    CMNumberSlider *lumaSlider;
    CMNumberSlider *chromaSlider;
    QComboBox *tmModeSelector;
    CMNumberSlider *gammaSlider;
    CMNumberSlider *shadowSlider;
    CMNumberSlider *blackSlider;
};

#endif // CMCONTROLSWIDGET_H
