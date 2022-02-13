#include "mainwindow.h"
#include "cmpicturelabel.h"
#include "cmnumberslider.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QImage>
#include <QScrollArea>
#include <QGroupBox>
#include <QComboBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *cw = new QWidget(this);
    this->setCentralWidget(cw);

    QHBoxLayout *hbl = new QHBoxLayout(cw);

    this->imgLabel = new CMPictureLabel(cw);
    hbl->addWidget(this->imgLabel, 1);

    this->imgLabel->setMinimumWidth(480);
    this->imgLabel->setMinimumHeight(360);

    QWidget *ctrlWidget = new QWidget(cw);
    QVBoxLayout *vbl = new QVBoxLayout(ctrlWidget);
    hbl->addWidget(ctrlWidget);

    QScrollArea *ctrlScrollRegionWidget = new QScrollArea(ctrlWidget);
    QPushButton *shootButton = new QPushButton(ctrlWidget);
    vbl->addWidget(ctrlScrollRegionWidget, 1);
    vbl->addWidget(shootButton);

    ctrlScrollRegionWidget->setFixedWidth(300);
    setupControls(ctrlScrollRegionWidget);

    shootButton->setText(tr("Shoot"));
}

MainWindow::~MainWindow()
{
    ;
}

void MainWindow::setupControls(QScrollArea *controlArea)
{
    QWidget *controlsWidget = new QWidget(controlArea);
    QVBoxLayout *cvl = new QVBoxLayout(controlsWidget);
    controlsWidget->setFixedWidth(controlArea->width() - 4);

    QGroupBox *exposureGroup = new QGroupBox(tr("Exposure"), controlsWidget);
    QGroupBox *wbGroup = new QGroupBox(tr("White Balance"), controlsWidget);
    QGroupBox *colourGroup = new QGroupBox(tr("Colour"), controlsWidget);
    QGroupBox *nrGroup = new QGroupBox(tr("Noise Reduction"), controlsWidget);
    QGroupBox *tmapGroup = new QGroupBox(tr("Tone Mapping"), controlsWidget);

    cvl->addWidget(exposureGroup);
    cvl->addWidget(wbGroup);
    cvl->addWidget(colourGroup);
    cvl->addWidget(nrGroup);
    cvl->addWidget(tmapGroup);
    cvl->addStretch(1);

    QGridLayout *egl = new QGridLayout(exposureGroup);
    egl->setColumnMinimumWidth(0, 60);
    egl->setColumnStretch(1, 1);
    QLabel *expLabel = new QLabel(tr("Exposure"), exposureGroup);
    CMNumberSlider *expSlider = new CMNumberSlider(exposureGroup);
    expSlider->setMinMax(0, 3);
    egl->addWidget(expLabel, 0, 0);
    egl->addWidget(expSlider, 0, 1);

    QGridLayout *wbgl = new QGridLayout(wbGroup);
    wbgl->setColumnMinimumWidth(0, 60);
    wbgl->setColumnStretch(1, 1);
    QLabel *warmthLabel = new QLabel(tr("Warmth"), wbGroup);
    CMNumberSlider *warmthSlider = new CMNumberSlider(wbGroup);
    wbgl->addWidget(warmthLabel, 0, 0);
    wbgl->addWidget(warmthSlider, 0, 1);
    QLabel *tintLabel = new QLabel(tr("Tint"), wbGroup);
    CMNumberSlider *tintSlider = new CMNumberSlider(wbGroup);
    wbgl->addWidget(tintLabel, 1, 0);
    wbgl->addWidget(tintSlider, 1, 1);

    QGridLayout *cgl = new QGridLayout(colourGroup);
    cgl->setColumnMinimumWidth(0, 60);
    cgl->setColumnStretch(1, 1);
    QLabel *hueLabel = new QLabel(tr("Hue"), colourGroup);
    CMNumberSlider *hueSlider = new CMNumberSlider(colourGroup);
    cgl->addWidget(hueLabel, 0, 0);
    cgl->addWidget(hueSlider, 0, 1);
    QLabel *satLabel = new QLabel(tr("Satuation"), colourGroup);
    CMNumberSlider *satSlider = new CMNumberSlider(colourGroup);
    cgl->addWidget(satLabel, 1, 0);
    cgl->addWidget(satSlider, 1, 1);

    QGridLayout *nrgl = new QGridLayout(nrGroup);
    nrgl->setColumnMinimumWidth(0, 60);
    nrgl->setColumnStretch(1, 1);
    QLabel *lumaLabel = new QLabel(tr("Luma"), nrGroup);
    CMNumberSlider *lumaSlider = new CMNumberSlider(nrGroup);
    nrgl->addWidget(lumaLabel, 0, 0);
    nrgl->addWidget(lumaSlider, 0, 1);
    QLabel *chromaLabel = new QLabel(tr("Chroma"), nrGroup);
    CMNumberSlider *chromaSlider = new CMNumberSlider(nrGroup);
    nrgl->addWidget(chromaLabel, 1, 0);
    nrgl->addWidget(chromaSlider, 1, 1);

    QGridLayout *tmgl = new QGridLayout(tmapGroup);
    tmgl->setColumnMinimumWidth(0, 60);
    tmgl->setColumnStretch(1, 1);
    QLabel *tmModeLabel = new QLabel(tr("Mode"), tmapGroup);
    QComboBox *tmModeSelector = new QComboBox(tmapGroup);
    tmModeSelector->addItem(tr("Linear"), 0);
    tmModeSelector->addItem(tr("Filmic"), 1);
    tmModeSelector->addItem(tr("Cubic"), 2);
    tmgl->addWidget(tmModeLabel, 0, 0);
    tmgl->addWidget(tmModeSelector, 0, 1);
    QLabel *gammaLabel = new QLabel(tr("Gamma"), tmapGroup);
    CMNumberSlider *gammaSlider = new CMNumberSlider(tmapGroup);
    tmgl->addWidget(gammaLabel, 1, 0);
    tmgl->addWidget(gammaSlider, 1, 1);
    QLabel *shadowLabel = new QLabel(tr("Shadow"), tmapGroup);
    CMNumberSlider *shadowSlider = new CMNumberSlider(tmapGroup);
    tmgl->addWidget(shadowLabel, 2, 0);
    tmgl->addWidget(shadowSlider, 2, 1);

    controlArea->setWidget(controlsWidget);
}
