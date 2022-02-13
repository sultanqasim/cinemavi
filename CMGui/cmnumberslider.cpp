#include "cmnumberslider.h"
#include <QHBoxLayout>

static const int NUM_STEPS = 100;

CMNumberSlider::CMNumberSlider(QWidget *parent)
    : QWidget{parent}
{
    QHBoxLayout *hbl = new QHBoxLayout(this);
    hbl->setContentsMargins(0, 0, 0, 0);
    this->slider = new QSlider(Qt::Horizontal, this);
    this->spin = new QDoubleSpinBox(this);
    hbl->addWidget(this->slider, 1);
    hbl->addWidget(this->spin, 0);

    this->minVal = 0;
    this->step = 1.0 / NUM_STEPS;
    this->spin->setMinimum(this->minVal);
    this->spin->setMaximum(this->minVal + this->step*NUM_STEPS);
    this->spin->setSingleStep(0.01);
    this->slider->setMinimum(0);
    this->slider->setMaximum(NUM_STEPS);

    connect(this->spin, &QDoubleSpinBox::valueChanged, this, &CMNumberSlider::onSpin);
    connect(this->slider, &QSlider::valueChanged, this, &CMNumberSlider::onSlide);
}

void CMNumberSlider::onSpin(double d) {
    int tick = (d - this->minVal) / this->step;
    this->slider->setValue(tick);
    emit valueChanged(d);
}

void CMNumberSlider::onSlide(int i) {
    double d = this->minVal + this->step*i;
    this->spin->setValue(d);
    emit valueChanged(d);
}

void CMNumberSlider::setMinMax(double minVal, double maxVal)
{
    this->minVal = minVal;
    this->step = (maxVal - minVal) / NUM_STEPS;
    this->spin->setMinimum(minVal);
    this->spin->setMaximum(maxVal);
}
