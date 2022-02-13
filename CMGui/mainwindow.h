#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <cstdint>
#include <QMainWindow>
#include <QLabel>
#include <QPixmap>
#include <QScrollArea>
#include "cmpicturelabel.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    CMPictureLabel *imgLabel;

    void setupControls(QScrollArea *controlArea);
};
#endif // MAINWINDOW_H
