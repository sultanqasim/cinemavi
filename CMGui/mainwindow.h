#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <cstdint>
#include <QMainWindow>
#include <QLabel>
#include <QPixmap>
#include <QScrollArea>
#include "cmpicturelabel.h"
#include "cmcontrolswidget.h"
#include "cmrenderqueue.h"

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
    void onSaveImage();
    void onAutoWhiteBalance();

private:
    Ui::MainWindow *ui;
    CMPictureLabel *imgLabel;
    CMControlsWidget *controls;
    CMRenderQueue *renderQueue;
};
#endif // MAINWINDOW_H
