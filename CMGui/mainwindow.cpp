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
#include <cstdlib>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *cw = new QWidget(this);
    this->setCentralWidget(cw);

    QHBoxLayout *hbl = new QHBoxLayout(cw);

    this->imgLabel = new CMPictureLabel(cw);
    hbl->addWidget(this->imgLabel, 1);

    this->imgLabel->setMinimumWidth(640);
    this->imgLabel->setMinimumHeight(480);

    QWidget *ctrlWidget = new QWidget(cw);
    QVBoxLayout *vbl = new QVBoxLayout(ctrlWidget);
    hbl->addWidget(ctrlWidget);

    QScrollArea *ctrlScrollRegionWidget = new QScrollArea(ctrlWidget);
    QPushButton *shootButton = new QPushButton(ctrlWidget);
    vbl->addWidget(ctrlScrollRegionWidget, 1);
    vbl->addWidget(shootButton);

    ctrlScrollRegionWidget->setFixedWidth(400);
    this->controls = new CMControlsWidget(ctrlScrollRegionWidget);
    ctrlScrollRegionWidget->setWidget(this->controls);
    connect(this->controls, &CMControlsWidget::paramsChanged, this, &MainWindow::onParamsChanged);

    shootButton->setText(tr("Shoot"));

    this->renderQueue = new CMRenderQueue(this);
    connect(this->renderQueue, &CMRenderQueue::imageRendered, this->imgLabel, &CMPictureLabel::setPixmap);

    // TODO: dynamically load user selected image
    void *raw;
    CMRawHeader cmrh;
    int rawStat = cmraw_load(&raw, &cmrh, "/Users/sultan/Documents/cinemavi/test_ae23.cmr");
    if (rawStat == 0) {
        this->renderQueue->setImage(raw, &cmrh.cinfo);
        free(raw);

        // TODO: don't hard code
    }

    // TODO: don't hard code camera calibration
    // Matrix to convert from camera RGB to sRGB in D65 daylight illumination
    const ColourMatrix default_calib = {.m={
         1.75883, -0.68132,  0.01113,
        -0.58876,  1.49340, -0.55559,
         0.04679, -0.59206,  2.02246
    }};
    this->renderQueue->setCalib(default_calib);

    this->onParamsChanged(); // force a render
}

MainWindow::~MainWindow()
{
    ;
}

void MainWindow::onParamsChanged(void) {
    ImagePipelineParams params;
    this->controls->getParams(&params);
    this->renderQueue->setParams(params);
}
