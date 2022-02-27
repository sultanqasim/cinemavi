#ifndef CMNUMBERSLIDER_H
#define CMNUMBERSLIDER_H

#include <QWidget>
#include <QSlider>
#include <QDoubleSpinBox>

class CMNumberSlider : public QWidget
{
    Q_OBJECT
public:
    explicit CMNumberSlider(QWidget *parent = nullptr);
    void setMinMax(double minVal, double maxVal, bool logScale = false);
    double value() const;
    void setValue(double val);

private:
    QDoubleSpinBox *spin;
    QSlider *slider;
    double minVal;
    double step;
    double invLnStep;
    bool logScale;

signals:
    double valueChanged(double v);

public slots:
    void onSpin(double d);
    void onSlide(int i);
};

#endif // CMNUMBERSLIDER_H
