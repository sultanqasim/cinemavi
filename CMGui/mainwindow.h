#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <cstdint>
#include "cmcamerainterface.h"
#include <QMainWindow>
#include <QLabel>
#include <QPixmap>
#include <QScrollArea>
#include <QFileInfo>
#include "cmpicturelabel.h"
#include "cmcontrolswidget.h"
#include "cmrenderqueue.h"
#include "cmautoexposure.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void onParamsChanged();
    void onOpenRaw();
    void onOpenCamera();
    void onSaveImage();
    void onAutoWhiteBalance(CMAutoWhiteMode mode);
    void onPicturePressed(uint16_t posX, uint16_t posY);
    void onImageCaptured(const CMRawImage &img);
    void onExposureUpdate(double changeFactor);

private:
    Ui::MainWindow *ui;
    CMPictureLabel *imgLabel;
    CMControlsWidget *controls;
    CMRenderQueue *renderQueue;
    CMCameraInterface *cameraInterface;
    CMAutoExposure *autoExposure;
    QFileInfo rawFileInfo;
};
#endif // MAINWINDOW_H
