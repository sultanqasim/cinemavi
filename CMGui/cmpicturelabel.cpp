#include "cmpicturelabel.h"

#include <QResizeEvent>
#include <QImage>
#include <QFile>
#include <QColorSpace>

CMPictureLabel::CMPictureLabel(QWidget *parent) :
    QWidget(parent)
{
    this->loadDisplayColourTransform();
    imgLabel = new QLabel(this);
    imgLabel->setAlignment(Qt::AlignCenter);
    imgLabel->setPixmap(imgMap);
}

CMPictureLabel::~CMPictureLabel() {}

void CMPictureLabel::resizeEvent(QResizeEvent *event)
{
    this->regenPixmap(event->size());
    imgLabel->resize(event->size());
    QWidget::resizeEvent(event);
}

void CMPictureLabel::mousePressEvent(QMouseEvent *event)
{
    QPoint labelClickPos = this->imgLabel->mapFromParent(event->pos());

    double labWidth = this->imgLabel->width();
    double labHeight = this->imgLabel->height();
    double imgWidth = this->imgMap.width();
    double imgHeight = this->imgMap.height();

    if (imgWidth > 0) {
        double imgShapeFactor = imgHeight / imgWidth;
        double labelShapeFactor = labHeight / labWidth;

        double originX = 0;
        double originY = 0;
        double imgPixPerScreen;
        if (labelShapeFactor > imgShapeFactor) {
            // label is taller than image
            double labPmHeight = this->imgLabel->pixmap().height();
            originY = (labHeight - labPmHeight) * 0.5;
            imgPixPerScreen = imgWidth / labWidth;
        } else {
            // label is wider than image
            double labPmWidth = this->imgLabel->pixmap().width();
            originX = (labWidth - labPmWidth) * 0.5;
            imgPixPerScreen = imgHeight / labHeight;
        }

        double imgX = imgPixPerScreen * (labelClickPos.x() - originX);
        double imgY = imgPixPerScreen * (labelClickPos.y() - originY);

        if (imgX >= 0 && imgX < imgWidth && imgY >= 0 && imgY < imgHeight)
            emit picturePressed(imgX, imgY);
    }

    QWidget::mousePressEvent(event);
}

void CMPictureLabel::setImage(const QImage &img) {
    if (this->colourTransformValid) {
        QImage xfmImg = img;
        xfmImg.applyColorTransform(this->colourTransform);
        this->imgMap.convertFromImage(xfmImg);
    } else {
        this->imgMap.convertFromImage(img);
    }
    this->regenPixmap(this->imgLabel->size());
}

void CMPictureLabel::regenPixmap(const QSize &size) {
    if (!this->imgMap.isNull()) {
        QPixmap pm = this->imgMap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        this->imgLabel->setPixmap(pm);
    } else {
        this->imgLabel->setPixmap(this->imgMap);
    }
}

#ifdef __APPLE__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Welaborated-enum-base"
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#pragma GCC diagnostic pop

typedef struct {
    CFUUIDRef dispuuid;
    CFURLRef url;
} ColorSyncIteratorData;

static bool colorSyncIterateCallback(CFDictionaryRef dict, void *data)
{
    ColorSyncIteratorData *iterData = (ColorSyncIteratorData *)data;
    CFStringRef str;
    CFUUIDRef uuid;
    CFBooleanRef iscur;

    if (!CFDictionaryGetValueIfPresent(dict, kColorSyncDeviceClass, (const void**)&str))
    {
        qWarning("kColorSyncDeviceClass failed");
        return true;
    }
    if (!CFEqual(str, kColorSyncDisplayDeviceClass))
    {
        return true;
    }
    if (!CFDictionaryGetValueIfPresent(dict, kColorSyncDeviceID, (const void**)&uuid))
    {
        qWarning("kColorSyncDeviceID failed");
        return true;
    }
    if (!CFEqual(uuid, iterData->dispuuid))
    {
        return true;
    }
    if (!CFDictionaryGetValueIfPresent(dict, kColorSyncDeviceProfileIsCurrent, (const void**)&iscur))
    {
        qWarning("kColorSyncDeviceProfileIsCurrent failed");
        return true;
    }
    if (!CFBooleanGetValue(iscur))
    {
        return true;
    }
    if (!CFDictionaryGetValueIfPresent(dict, kColorSyncDeviceProfileURL, (const void**)&(iterData->url)))
    {
        qWarning("Could not get current profile URL");
        return true;
    }

    CFRetain(iterData->url);
    return false;
}

#endif

QString CMPictureLabel::getDisplayProfileURL()
{
#ifdef __APPLE__
    ColorSyncIteratorData data;
    data.dispuuid = CGDisplayCreateUUIDFromDisplayID(CGMainDisplayID());
    data.url = NULL;
    ColorSyncIterateDeviceProfiles(colorSyncIterateCallback, (void *)&data);
    CFRelease(data.dispuuid);
    CFStringRef urlstr = CFURLCopyFileSystemPath(data.url, kCFURLPOSIXPathStyle);
    CFRelease(data.url);
    return QString::fromCFString(urlstr);
#else
    // TODO: handle colord on Linux and equivalent on Windows
    return QString();
#endif
}

void CMPictureLabel::loadDisplayColourTransform()
{
    this->colourTransformValid = false;
    QString profileURL = getDisplayProfileURL();
    if (profileURL.isEmpty()) return;

    QFile iccProfileFile(profileURL);
    if (!iccProfileFile.open(QIODevice::ReadOnly)) return;
    QByteArray iccProfile = iccProfileFile.readAll();

    QColorSpace displaySpace = QColorSpace::fromIccProfile(iccProfile);
    QColorSpace sRGB = QColorSpace(QColorSpace::SRgb);

    this->colourTransform = sRGB.transformationToColorSpace(displaySpace);
    this->colourTransformValid = true;
}
