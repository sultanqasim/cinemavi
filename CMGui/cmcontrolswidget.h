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

signals:
    void paramsChanged();

private slots:
    void onLUTModeChanged(int index);
    void onSliderChanged(double val);
    void onReset(void);

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
