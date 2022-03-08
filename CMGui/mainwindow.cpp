#include "mainwindow.h"
#include "cmpicturelabel.h"
#include "cmnumberslider.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QAction>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
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
    connect(this->imgLabel, &CMPictureLabel::picturePressed, this, &MainWindow::onPicturePressed);

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
    connect(this->controls, &CMControlsWidget::autoWhiteBalanceTriggered,
            this, &MainWindow::onAutoWhiteBalance);

    shootButton->setText(tr("Shoot"));
    shootButton->setHidden(true); // should only be visible when camera is running

    this->renderQueue = new CMRenderQueue(this);
    connect(this->renderQueue, &CMRenderQueue::imageRendered, this->imgLabel, &CMPictureLabel::setPixmap);

    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *openAction = new QAction(tr("&Open CMRAW..."), this);
    fileMenu->addAction(openAction);
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenRaw);
    QAction *saveAction = new QAction(tr("&Save image..."), this);
    fileMenu->addAction(saveAction);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveImage);

    this->setWindowTitle(tr("Cinemavi"));

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

void MainWindow::onParamsChanged()
{
    ImagePipelineParams params;
    this->controls->getParams(&params);
    this->renderQueue->setParams(params);
}

void MainWindow::onOpenRaw()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open CMRAW Image"), "",
                                                    tr("CMRAW Files (*.cmr)"));
    if (fileName.isNull())
        return;
    std::string cmrFileName = fileName.toStdString();

    void *raw;
    CMRawHeader cmrh;
    int rawStat = cmraw_load(&raw, &cmrh, cmrFileName.c_str());
    if (rawStat == 0) {
        this->renderQueue->setImage(raw, &cmrh.cinfo);
        free(raw);

        // strip the full path
        this->rawFileInfo.setFile(fileName);
        this->setWindowTitle(tr("Cinemavi") + " - " + this->rawFileInfo.fileName());
    } else {
        QMessageBox::critical(this, "", tr("Error opening file"));
    }
}

void MainWindow::onSaveImage()
{
    // only makes sense when a raw is loaded
    if (this->rawFileInfo.filePath().isEmpty())
        return;

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image"),
            this->rawFileInfo.baseName() + ".tiff", tr("TIFF Files (*.tiff)"));
    if (fileName.isNull())
        return;
    this->renderQueue->saveImage(fileName);
}

void MainWindow::onAutoWhiteBalance(CMAutoWhiteMode mode)
{
    CMAutoWhiteParams params = {.awb_mode=mode};
    double temp_K, tint;
    if (this->renderQueue->autoWhiteBalance(params, &temp_K, &tint))
        this->controls->setWhiteBalance(temp_K, tint);
}

void MainWindow::onPicturePressed(uint16_t posX, uint16_t posY)
{
    if (this->controls->spotWhiteChecked()) {
        CMAutoWhiteParams params = {.awb_mode=CMWHITE_SPOT, .pos_x=posX, .pos_y=posY};
        double temp_K, tint;
        if (this->renderQueue->autoWhiteBalance(params, &temp_K, &tint))
            this->controls->setWhiteBalance(temp_K, tint);
    }
}
