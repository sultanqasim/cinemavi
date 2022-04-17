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
#include <QDateTime>
#include <QDir>
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
    this->camControls = new CMCameraControls(this);
    vbl->addWidget(ctrlScrollRegionWidget, 1);
    vbl->addWidget(camControls);

    ctrlScrollRegionWidget->setFixedWidth(400);
    ctrlScrollRegionWidget->setMinimumHeight(240);
    this->controls = new CMControlsWidget(ctrlScrollRegionWidget);
    ctrlScrollRegionWidget->setWidget(this->controls);
    connect(this->controls, &CMControlsWidget::paramsChanged, this, &MainWindow::onParamsChanged);
    connect(this->controls, &CMControlsWidget::autoWhiteBalanceTriggered,
            this, &MainWindow::onAutoWhiteBalance);

    this->camControls->setFixedWidth(400);
    this->camControls->setHidden(true); // only visible when camera is running
    connect(this->camControls, &CMCameraControls::shootClicked, this, &MainWindow::onShoot);
    connect(this->camControls, &CMCameraControls::exposureChanged,
            this, &MainWindow::onExposureChanged);

    this->renderQueue = new CMRenderQueue(this);
    connect(this->renderQueue, &CMRenderQueue::imageRendered,
            this->imgLabel, &CMPictureLabel::setPixmap);
    connect(this->renderQueue, &CMRenderQueue::imageSaved,
            this, &MainWindow::onSaveDone);

    this->cameraInterface = new CMCameraInterface();
    connect(this->cameraInterface, &CMCameraInterface::imageCaptured,
            this, &MainWindow::onImageCaptured);

    this->autoExposure = new CMAutoExposure();
    connect(this->autoExposure, &CMAutoExposure::exposureChangeCalculated,
            this, &MainWindow::onExposureUpdate);

    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *openAction = new QAction(tr("&Open CMRAW..."), this);
    fileMenu->addAction(openAction);
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenRaw);
    QAction *openCameraAction = new QAction(tr("Open camera"), this);
    fileMenu->addAction(openCameraAction);
    connect(openCameraAction, &QAction::triggered, this, &MainWindow::onOpenCamera);
    this->saveAction = new QAction(tr("&Save image..."), this);
    fileMenu->addAction(saveAction);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveImage);
    QAction *closeAction = new QAction(tr("&Close image/camera"), this);
    fileMenu->addAction(closeAction);
    connect(closeAction, &QAction::triggered, this, &MainWindow::onClose);

    this->setWindowTitle(tr("Cinemavi"));

    this->onParamsChanged(); // force a render
}

MainWindow::~MainWindow()
{
    delete this->cameraInterface;
    delete this->autoExposure;
}

void MainWindow::onParamsChanged()
{
    ImagePipelineParams params;
    this->controls->getParams(&params);
    this->renderQueue->setParams(params);
    this->autoExposure->setParams(params);
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
        CMRawImage img;
        this->cameraInterface->stopCapture();
        this->camControls->setHidden(true);
        img.setImage(raw, cmrh);
        this->renderQueue->setImageLater(img);
        free(raw);

        if (cmrh.cinfo.white_x > 0 || cmrh.cinfo.white_y > 0) {
            double temp_K, tint;
            colour_xy_to_temp_tint(cmrh.cinfo.white_x, cmrh.cinfo.white_y, &temp_K, &tint);
            this->controls->setShotWhiteBalance(temp_K, tint);
        } else {
            this->controls->setShotWhiteBalance();
        }

        // strip the full path
        this->rawFileInfo.setFile(fileName);
        this->setWindowTitle(tr("Cinemavi") + " - " + this->rawFileInfo.fileName());
    } else {
        QMessageBox::critical(this, "", tr("Error opening file"));
    }
}

void MainWindow::onOpenCamera()
{
    if (!this->cameraInterface->cameraAvailable()) {
        QMessageBox::critical(this, "", tr("Error opening camera"));
        return;
    }
    this->cameraInterface->setExposure(30000, 5);
    this->cameraInterface->setFrameRate(16);
    this->cameraInterface->startCapture();
    this->controls->setShotWhiteBalance();
    this->setWindowTitle(tr("Cinemavi") + " - " + this->cameraInterface->getCameraName());
    this->rawFileInfo.setFile("");
    this->camControls->setExposureLimits(this->cameraInterface->getExposureLimits());
    this->camControls->setHidden(false);
}

void MainWindow::onSaveImage()
{
    // only makes sense when there's an image to save
    if (!this->renderQueue->hasImage())
        return;

    QString baseName;
    if (this->rawFileInfo.filePath().isEmpty()) {
        QDateTime t = QDateTime::currentDateTime();
        baseName = "CMIMG_" + t.toString("yyyy-MM-dd_hh-mm-ss");
    } else
        baseName = this->rawFileInfo.baseName();

    /* Crashes on Mac due to Mac OS or QT bug it seems when image formats are part of list
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image"),
            baseName + ".jpg", tr("CMRAW Files (*.cmr);;DNG Files (*.dng);;TIFF Files (*.tiff);;JPEG Files (*.jpg)"));
            */
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image"),
            baseName + ".tiff", tr("Image Files (*.cmr *.dng *.tiff *.jpg)"));
    if (fileName.isNull())
        return;
    this->saveAction->setEnabled(false);
    this->camControls->setShootEnabled(false);
    this->renderQueue->saveImage(fileName);
}

void MainWindow::onShoot()
{
    QString saveDir = this->camControls->saveDir();
    if (!QDir().mkpath(saveDir)) {
        QMessageBox::critical(this, "", tr("Error creating save directory"));
        return;
    }

    QString suffix;
    switch(this->camControls->captureFormat()) {
    case CMCAP_CMRAW:
        suffix = ".cmr";
        break;
    case CMCAP_DNG:
        suffix = ".dng";
        break;
    case CMCAP_TIFF:
        suffix = ".tiff";
        break;
    case CMCAP_JPEG:
        suffix = ".jpg";
        break;
    }

    QDateTime t = QDateTime::currentDateTime();
    QString baseName = "CMIMG_" + t.toString("yyyy-MM-dd_hh-mm-ss");
    QString fileName = saveDir + "/" + baseName + suffix;
    this->saveAction->setEnabled(false);
    this->camControls->setShootEnabled(false);
    this->renderQueue->saveImage(fileName);
}

void MainWindow::onSaveDone(bool success)
{
    this->saveAction->setEnabled(true);
    this->camControls->setShootEnabled(true);

    if (!success)
        QMessageBox::critical(this, "", tr("Error saving image"));
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

void MainWindow::onImageCaptured(const CMRawImage &img)
{
    this->renderQueue->setImage(img);

    if (this->camControls->exposureMode() != CMEXP_MANUAL)
        this->autoExposure->setImage(img);
}

void MainWindow::onExposureUpdate(double changeFactor)
{
    double shutter_us, gain_dB;
    this->cameraInterface->updateExposure(this->camControls->exposureMode(), changeFactor);
    this->cameraInterface->getExposure(&shutter_us, &gain_dB);
    this->camControls->setShutter(shutter_us);
    this->camControls->setGain(gain_dB);
}

void MainWindow::onExposureChanged(CMExposureMode mode, double shutter_us, double gain_dB)
{
    if (mode != CMEXP_AUTO)
        this->cameraInterface->setExposure(shutter_us, gain_dB);
}

void MainWindow::onClose()
{
    CMRawImage emptyImg;
    this->cameraInterface->stopCapture();
    this->camControls->setHidden(true);
    this->renderQueue->setImageLater(emptyImg);
    this->controls->setShotWhiteBalance();
    this->setWindowTitle(tr("Cinemavi"));
}
