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

    shootButton->setText(tr("Shoot"));
}

MainWindow::~MainWindow()
{
    ;
}
