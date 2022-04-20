#include "cmcontrolswidget.h"
#include <QGroupBox>
#include <QHBoxLayout>
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
    tintSlider->setMinMax(-1, 1);
    wbgl->addWidget(tintLabel, 1, 0);
    wbgl->addWidget(tintSlider, 1, 1);
    QLabel *awbLabel = new QLabel(tr("Auto"), wbGroup);
    QWidget *autoButtons = new QWidget(wbGroup);
    QHBoxLayout *autoButtonsLayout = new QHBoxLayout(autoButtons);
    autoButtonsLayout->setContentsMargins(0, 0, 0, 0);
    QPushButton *brightsWhiteButton = new QPushButton(tr("Brights"), autoButtons);
    QPushButton *robustWhiteButton = new QPushButton(tr("Robust"), autoButtons);
    spotWhiteButton = new QPushButton(tr("Spot"), autoButtons);
    spotWhiteButton->setCheckable(true);
    autoButtonsLayout->addWidget(brightsWhiteButton);
    autoButtonsLayout->addWidget(robustWhiteButton);
    autoButtonsLayout->addWidget(spotWhiteButton);
    wbgl->addWidget(awbLabel, 2, 0);
    wbgl->addWidget(autoButtons, 2, 1);

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
    satSlider->setMinMax(0.3, 1.3);
    cgl->addWidget(satLabel, 1, 0);
    cgl->addWidget(satSlider, 1, 1);

    QGridLayout *nrgl = new QGridLayout(nrGroup);
    nrgl->setColumnMinimumWidth(0, 60);
    nrgl->setColumnStretch(1, 1);
    QLabel *debayerLabel = new QLabel(tr("Debayer"), nrGroup);
    debayerModeSelector = new QComboBox(nrGroup);
    debayerModeSelector->addItem(tr("2x2 Nearest Neighbour"), CMBAYER_22);
    debayerModeSelector->addItem(tr("3x3 Bilinear"), CMBAYER_33);
    debayerModeSelector->addItem(tr("5x5 Chroma Smoothing"), CMBAYER_55);
    debayerModeSelector->addItem(tr("5x5 VNG"), CMBAYER_55_VNG);
    nrgl->addWidget(debayerLabel, 0, 0);
    nrgl->addWidget(debayerModeSelector, 0, 1);
    QLabel *nrModeLabel = new QLabel("Mode", nrGroup);
    nrModeSelector = new QComboBox(nrGroup);
    nrModeSelector->addItem(tr("None"), CMNR_NONE);
    nrModeSelector->addItem(tr("Gaussian Blur"), CMNR_GAUSSIAN);
    nrModeSelector->addItem(tr("Median Filter"), CMNR_MEDIAN);
    nrModeSelector->addItem(tr("Strong Median Filter"), CMNR_MEDIAN_STRONG);
    nrgl->addWidget(nrModeLabel, 1, 0);
    nrgl->addWidget(nrModeSelector, 1, 1);
    QLabel *lumaLabel = new QLabel(tr("Luma"), nrGroup);
    lumaSlider = new CMNumberSlider(nrGroup);
    lumaSlider->setMinMax(-100, 0);
    nrgl->addWidget(lumaLabel, 2, 0);
    nrgl->addWidget(lumaSlider, 2, 1);
    QLabel *chromaLabel = new QLabel(tr("Chroma"), nrGroup);
    chromaSlider = new CMNumberSlider(nrGroup);
    chromaSlider->setMinMax(-100, 0);
    nrgl->addWidget(chromaLabel, 3, 0);
    nrgl->addWidget(chromaSlider, 3, 1);

    QGridLayout *tmgl = new QGridLayout(tmapGroup);
    tmgl->setColumnMinimumWidth(0, 60);
    tmgl->setColumnStretch(1, 1);
    QLabel *tmModeLabel = new QLabel(tr("Mode"), tmapGroup);
    tmModeSelector = new QComboBox(tmapGroup);
    tmModeSelector->addItem(tr("Linear"), CMLUT_LINEAR);
    tmModeSelector->addItem(tr("Filmic"), CMLUT_FILMIC);
    tmModeSelector->addItem(tr("Cubic"), CMLUT_CUBIC);
    tmModeSelector->addItem(tr("HDR"), CMLUT_HDR);
    tmModeSelector->addItem(tr("HDR Auto"), CMLUT_HDR_AUTO);
    tmModeSelector->addItem(tr("HDR Cubic"), CMLUT_HDR_CUBIC);
    tmModeSelector->addItem(tr("HDR Cubic Auto"), CMLUT_HDR_CUBIC_AUTO);
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
    connect(this->debayerModeSelector, &QComboBox::currentIndexChanged, this, &CMControlsWidget::onDebayerModeChanged);
    connect(this->nrModeSelector, &QComboBox::currentIndexChanged, this, &CMControlsWidget::onNRModeChanged);
    connect(this->tmModeSelector, &QComboBox::currentIndexChanged, this, &CMControlsWidget::onLUTModeChanged);
    connect(brightsWhiteButton, &QPushButton::clicked, this, &CMControlsWidget::onBrightsWhiteBalance);
    connect(robustWhiteButton, &QPushButton::clicked, this, &CMControlsWidget::onRobustWhiteBalance);
    connect(resetButton, &QPushButton::clicked, this, &CMControlsWidget::onReset);

    this->onReset();
}

void CMControlsWidget::onReset(void)
{
    expSlider->setValue(0);
    warmthSlider->setValue(this->shotTempK);
    tintSlider->setValue(this->shotTint);
    hueSlider->setValue(0);
    satSlider->setValue(1);
    lumaSlider->setValue(-27);
    chromaSlider->setValue(-25);
    gammaSlider->setValue(0.3);
    shadowSlider->setValue(1);
    blackSlider->setValue(0.25);
    debayerModeSelector->setCurrentIndex(CMBAYER_33);
    nrModeSelector->setCurrentIndex(CMNR_MEDIAN);
    tmModeSelector->setCurrentIndex(CMLUT_HDR_CUBIC_AUTO);
}

void CMControlsWidget::onDebayerModeChanged(int index)
{
    (void)index; // use unused argument
    emit paramsChanged();
}

void CMControlsWidget::onNRModeChanged(int index)
{
    CMNoiseReductionMode nrm = (CMNoiseReductionMode)index;
    switch (nrm) {
    case CMNR_NONE:
        lumaSlider->setEnabled(false);
        chromaSlider->setEnabled(false);
        break;
    default:
        lumaSlider->setEnabled(true);
        chromaSlider->setEnabled(true);
        break;
    }

    emit paramsChanged();
}

void CMControlsWidget::onLUTModeChanged(int index)
{
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
        gammaSlider->setEnabled(false);
        shadowSlider->setEnabled(false);
        blackSlider->setEnabled(false);
        break;
    case CMLUT_HDR_CUBIC:
        gammaSlider->setEnabled(true);
        shadowSlider->setEnabled(true);
        blackSlider->setEnabled(true);
        break;
    case CMLUT_HDR_CUBIC_AUTO:
        gammaSlider->setEnabled(false);
        shadowSlider->setEnabled(false);
        blackSlider->setEnabled(false);
        break;
    }

    emit paramsChanged();
}

void CMControlsWidget::onSliderChanged(double val)
{
    (void)val;
    emit paramsChanged();
}

void CMControlsWidget::getParams(ImagePipelineParams *params)
{
    params->exposure = this->expSlider->value();
    params->temp_K = this->warmthSlider->value();
    params->tint = this->tintSlider->value();
    params->hue = this->hueSlider->value() * M_PI / 180.0;
    params->sat = this->satSlider->value();
    params->noise_lum_dB = this->lumaSlider->value();
    params->noise_chrom_dB = this->chromaSlider->value();
    params->gamma = this->gammaSlider->value();
    params->shadow = this->shadowSlider->value();
    params->black = this->blackSlider->value();
    params->lut_mode = (CMLUTMode)this->tmModeSelector->currentIndex();
    params->nr_mode = (CMNoiseReductionMode)this->nrModeSelector->currentIndex();
    params->debayer_mode = (CMDebayerMode)this->debayerModeSelector->currentIndex();
}

void CMControlsWidget::onBrightsWhiteBalance()
{
    emit autoWhiteBalanceTriggered(CMWHITE_BRIGHTS);
}

void CMControlsWidget::onRobustWhiteBalance()
{
    emit autoWhiteBalanceTriggered(CMWHITE_ROBUST);
}

void CMControlsWidget::setWhiteBalance(double temp_K, double tint)
{
    this->warmthSlider->setValue(temp_K);
    this->tintSlider->setValue(tint);
}

void CMControlsWidget::setShotWhiteBalance(double temp_K, double tint)
{
    this->shotTempK = temp_K;
    this->shotTint = tint;
    this->setWhiteBalance(temp_K, tint);
}

bool CMControlsWidget::spotWhiteChecked()
{
    return this->spotWhiteButton->isChecked();
}
