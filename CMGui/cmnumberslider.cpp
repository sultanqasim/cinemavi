#include "cmnumberslider.h"
#include <QHBoxLayout>
#include <cmath>

static const int NUM_STEPS = 200;
static const double EPSILON = 1E-6;

CMNumberSlider::CMNumberSlider(QWidget *parent)
    : QWidget{parent}
{
    QHBoxLayout *hbl = new QHBoxLayout(this);
    hbl->setContentsMargins(0, 0, 0, 0);
    this->slider = new QSlider(Qt::Horizontal, this);
    this->spin = new QDoubleSpinBox(this);
    this->spin->setMinimumWidth(60);
    hbl->addWidget(this->slider, 1);
    hbl->addWidget(this->spin, 0);

    this->logScale = false;
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
    int tick;
    if (this->logScale) {
        tick = EPSILON + log(d / this->minVal) * this->invLnStep;
    } else {
        tick = EPSILON + (d - this->minVal) / this->step;
    }
    if (tick != this->slider->value())
        this->slider->setValue(tick);
    emit valueChanged(d);
}

void CMNumberSlider::onSlide(int i) {
    double curVal, nextVal;
    if (this->logScale) {
        curVal = this->minVal * pow(this->step, i);
        nextVal = this->minVal * pow(this->step, i+1);
    } else {
        curVal = this->minVal + this->step*i;
        nextVal = this->minVal + this->step*(i+1);
    }

    double spinVal = this->spin->value();
    if ((spinVal >= nextVal) || (spinVal < curVal - EPSILON)) {
        this->spin->setValue(curVal);
        emit valueChanged(curVal);
    }
}

void CMNumberSlider::setMinMax(double minVal, double maxVal, bool logScale)
{
    this->minVal = minVal;
    this->spin->setMinimum(minVal);
    this->spin->setMaximum(maxVal);
    if (logScale) {
        // f(x) = a * b^x where a := this->minVal and b := this->step
        this->step = exp(log(maxVal / minVal) / NUM_STEPS);
        this->invLnStep = NUM_STEPS / log(maxVal / minVal);
        this->logScale = true;
    } else {
        this->logScale = false;
        this->step = (maxVal - minVal) / NUM_STEPS;
    }

    double rangeSize = maxVal - minVal;
    if (rangeSize >= 1000) {
        this->spin->setSingleStep(10);
        this->spin->setDecimals(0);
    } else if (rangeSize >= 100) {
        this->spin->setSingleStep(1);
        this->spin->setDecimals(1);
    } else if (rangeSize >= 2){
        this->spin->setSingleStep(0.1);
        this->spin->setDecimals(2);
    } else {
        this->spin->setSingleStep(0.01);
        this->spin->setDecimals(3);
    }

    this->onSpin(this->spin->value());
}

double CMNumberSlider::value() const {
    return this->spin->value();
}

void CMNumberSlider::setValue(double val) {
    this->spin->setValue(val);
}
