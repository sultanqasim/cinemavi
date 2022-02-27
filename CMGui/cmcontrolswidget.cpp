#include "cmcontrolswidget.h"
#include <QGroupBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

CMControlsWidget::CMControlsWidget(QWidget *parent)
    : QWidget{parent}
{
    QVBoxLayout *cvl = new QVBoxLayout(this);
    this->setFixedWidth(parent->width() - 4);

    QGroupBox *exposureGroup = new QGroupBox(tr("Exposure"), this);
    QGroupBox *wbGroup = new QGroupBox(tr("White Balance"), this);
    QGroupBox *colourGroup = new QGroupBox(tr("Colour"), this);
    QGroupBox *nrGroup = new QGroupBox(tr("Noise Reduction"), this);
    QGroupBox *tmapGroup = new QGroupBox(tr("Tone Mapping"), this);
    QPushButton *resetButton = new QPushButton(tr("Reset"), this);

    cvl->addWidget(exposureGroup);
    cvl->addWidget(wbGroup);
    cvl->addWidget(colourGroup);
    cvl->addWidget(nrGroup);
    cvl->addWidget(tmapGroup);
    cvl->addWidget(resetButton);

    QGridLayout *egl = new QGridLayout(exposureGroup);
    egl->setColumnMinimumWidth(0, 60);
    egl->setColumnStretch(1, 1);
    QLabel *expLabel = new QLabel(tr("Exposure"), exposureGroup);
    expSlider = new CMNumberSlider(exposureGroup);
    expSlider->setMinMax(-3, 3);
    egl->addWidget(expLabel, 0, 0);
    egl->addWidget(expSlider, 0, 1);

    QGridLayout *wbgl = new QGridLayout(wbGroup);
    wbgl->setColumnMinimumWidth(0, 60);
    wbgl->setColumnStretch(1, 1);
    QLabel *warmthLabel = new QLabel(tr("Warmth"), wbGroup);
    warmthSlider = new CMNumberSlider(wbGroup);
    warmthSlider->setMinMax(2500, 25000, true);
    wbgl->addWidget(warmthLabel, 0, 0);
    wbgl->addWidget(warmthSlider, 0, 1);
    QLabel *tintLabel = new QLabel(tr("Tint"), wbGroup);
    tintSlider = new CMNumberSlider(wbGroup);
    tintSlider->setMinMax(-2, 2);
    wbgl->addWidget(tintLabel, 1, 0);
    wbgl->addWidget(tintSlider, 1, 1);

    QGridLayout *cgl = new QGridLayout(colourGroup);
    cgl->setColumnMinimumWidth(0, 60);
    cgl->setColumnStretch(1, 1);
    QLabel *hueLabel = new QLabel(tr("Hue"), colourGroup);
    hueSlider = new CMNumberSlider(colourGroup);
    hueSlider->setMinMax(-180, 180); // degrees, must convert to radians
    cgl->addWidget(hueLabel, 0, 0);
    cgl->addWidget(hueSlider, 0, 1);
    QLabel *satLabel = new QLabel(tr("Satuation"), colourGroup);
    satSlider = new CMNumberSlider(colourGroup);
    satSlider->setMinMax(0, 2);
    cgl->addWidget(satLabel, 1, 0);
    cgl->addWidget(satSlider, 1, 1);

    QGridLayout *nrgl = new QGridLayout(nrGroup);
    nrgl->setColumnMinimumWidth(0, 60);
    nrgl->setColumnStretch(1, 1);
    QLabel *lumaLabel = new QLabel(tr("Luma"), nrGroup);
    lumaSlider = new CMNumberSlider(nrGroup);
    lumaSlider->setMinMax(0, 5000);
    nrgl->addWidget(lumaLabel, 0, 0);
    nrgl->addWidget(lumaSlider, 0, 1);
    QLabel *chromaLabel = new QLabel(tr("Chroma"), nrGroup);
    chromaSlider = new CMNumberSlider(nrGroup);
    chromaSlider->setMinMax(0, 5000);
    nrgl->addWidget(chromaLabel, 1, 0);
    nrgl->addWidget(chromaSlider, 1, 1);

    QGridLayout *tmgl = new QGridLayout(tmapGroup);
    tmgl->setColumnMinimumWidth(0, 60);
    tmgl->setColumnStretch(1, 1);
    QLabel *tmModeLabel = new QLabel(tr("Mode"), tmapGroup);
    tmModeSelector = new QComboBox(tmapGroup);
    tmModeSelector->addItem(tr("Linear"), 0);
    tmModeSelector->addItem(tr("Filmic"), 1);
    tmModeSelector->addItem(tr("Cubic"), 2);
    tmModeSelector->addItem(tr("HDR"), 3);
    tmModeSelector->addItem(tr("HDR Auto"), 4);
    tmModeSelector->addItem(tr("HDR Cubic"), 5);
    tmgl->addWidget(tmModeLabel, 0, 0);
    tmgl->addWidget(tmModeSelector, 0, 1);
    QLabel *gammaLabel = new QLabel(tr("Gamma"), tmapGroup);
    gammaSlider = new CMNumberSlider(tmapGroup);
    gammaSlider->setMinMax(0.005, 1, true);
    tmgl->addWidget(gammaLabel, 1, 0);
    tmgl->addWidget(gammaSlider, 1, 1);
    QLabel *shadowLabel = new QLabel(tr("Shadow"), tmapGroup);
    shadowSlider = new CMNumberSlider(tmapGroup);
    shadowSlider->setMinMax(1, 64, true);
    tmgl->addWidget(shadowLabel, 2, 0);
    tmgl->addWidget(shadowSlider, 2, 1);
    QLabel *blackLabel = new QLabel(tr("Black"), tmapGroup);
    blackSlider = new CMNumberSlider(tmapGroup);
    blackSlider->setMinMax(0, 3);
    tmgl->addWidget(blackLabel, 3, 0);
    tmgl->addWidget(blackSlider, 3, 1);

    connect(this->expSlider, &CMNumberSlider::valueChanged, this, &CMControlsWidget::onSliderChanged);
    connect(this->warmthSlider, &CMNumberSlider::valueChanged, this, &CMControlsWidget::onSliderChanged);
    connect(this->tintSlider, &CMNumberSlider::valueChanged, this, &CMControlsWidget::onSliderChanged);
    connect(this->hueSlider, &CMNumberSlider::valueChanged, this, &CMControlsWidget::onSliderChanged);
    connect(this->satSlider, &CMNumberSlider::valueChanged, this, &CMControlsWidget::onSliderChanged);
    connect(this->lumaSlider, &CMNumberSlider::valueChanged, this, &CMControlsWidget::onSliderChanged);
    connect(this->chromaSlider, &CMNumberSlider::valueChanged, this, &CMControlsWidget::onSliderChanged);
    connect(this->gammaSlider, &CMNumberSlider::valueChanged, this, &CMControlsWidget::onSliderChanged);
    connect(this->shadowSlider, &CMNumberSlider::valueChanged, this, &CMControlsWidget::onSliderChanged);
    connect(this->blackSlider, &CMNumberSlider::valueChanged, this, &CMControlsWidget::onSliderChanged);
    connect(this->tmModeSelector, &QComboBox::currentIndexChanged, this, &CMControlsWidget::onLUTModeChanged);
    connect(resetButton, &QPushButton::clicked, this, &CMControlsWidget::onReset);

    this->onReset();
}

void CMControlsWidget::onReset(void) {
    expSlider->setValue(0);
    warmthSlider->setValue(5000);
    tintSlider->setValue(0);
    hueSlider->setValue(0);
    satSlider->setValue(1);
    lumaSlider->setValue(0);
    chromaSlider->setValue(0);
    gammaSlider->setValue(0.2);
    shadowSlider->setValue(1);
    blackSlider->setValue(0.3);
    tmModeSelector->setCurrentIndex(CMLUT_HDR_CUBIC);
}

void CMControlsWidget::onLUTModeChanged(int index) {
    CMLUTMode lm = (CMLUTMode)index;
    switch (lm) {
    default:
    case CMLUT_LINEAR:
        gammaSlider->setEnabled(false);
        shadowSlider->setEnabled(false);
        blackSlider->setEnabled(false);
        break;
    case CMLUT_FILMIC:
        gammaSlider->setEnabled(true);
        shadowSlider->setEnabled(false);
        blackSlider->setEnabled(true);
        break;
    case CMLUT_CUBIC:
        gammaSlider->setEnabled(true);
        shadowSlider->setEnabled(false);
        blackSlider->setEnabled(true);
        break;
    case CMLUT_HDR:
        gammaSlider->setEnabled(true);
        shadowSlider->setEnabled(true);
        blackSlider->setEnabled(false);
        break;
    case CMLUT_HDR_AUTO:
        // Uses cubic parameters for low boost
        gammaSlider->setEnabled(true);
        shadowSlider->setEnabled(false);
        blackSlider->setEnabled(true);
        break;
    case CMLUT_HDR_CUBIC:
        gammaSlider->setEnabled(true);
        shadowSlider->setEnabled(true);
        blackSlider->setEnabled(true);
        break;
    }
}

void CMControlsWidget::onSliderChanged(double val) {
    (void)val;
    emit paramsChanged();
}
