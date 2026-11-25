#include "projectfile.h"
#include "layermanager.h"
#include "layer.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QBuffer>
#include <QImage>

namespace {

void writeString(QDataStream& s, const QString& str)
{
    const QByteArray utf8 = str.toUtf8();
    s << quint16(utf8.size());
    s.writeRawData(utf8.constData(), utf8.size());
}

QString readString(QDataStream& s)
{
    quint16 len = 0;
    s >> len;
    QByteArray buf(len, Qt::Uninitialized);
    s.readRawData(buf.data(), len);
    return QString::fromUtf8(buf);
}

QByteArray imageToPng(const QImage& img)
{
    QByteArray data;
    QBuffer buf(&data);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");
    return data;
}

QImage pngToImage(const QByteArray& data)
{
    QImage img;
    img.loadFromData(data, "PNG");
    return img;
}

} // namespace

bool ProjectFile::save(const QString& path, const LayerManager& mgr)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QDataStream s(&file);
    s.setVersion(QDataStream::Qt_6_0);
    s.setByteOrder(QDataStream::LittleEndian);

    s << quint32(kMagic);
    s << quint16(kVersion);
    s << quint16(0);
    s << quint32(mgr.width());
    s << quint32(mgr.height());
    s << quint32(mgr.layerCount());

    writeString(s, QFileInfo(path).completeBaseName());

    for (int i = 0; i < mgr.layerCount(); ++i) {
        Layer* layer = const_cast<LayerManager&>(mgr).layer(i);
        if (!layer) continue;

        writeString(s, layer->name());
        s << quint8(layer->isVisible() ? 1 : 0);
        s << quint8(layer->alphaLocked() ? 1 : 0);
        s << quint8(static_cast<quint8>(layer->blendMode()));
        s << float(layer->opacity());

        const QByteArray png = imageToPng(layer->image());
        s << quint32(png.size());
        s.writeRawData(png.constData(), png.size());
    }

    file.close();
    return s.status() == QDataStream::Ok;
}

bool ProjectFile::load(const QString& path, LayerManager& mgr)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QDataStream s(&file);
    s.setVersion(QDataStream::Qt_6_0);
    s.setByteOrder(QDataStream::LittleEndian);

    quint32 magic = 0;
    quint16 version = 0, flags = 0;
    quint32 width = 0, height = 0, layerCount = 0;

    s >> magic >> version >> flags >> width >> height >> layerCount;

    if (magic != kMagic || version != kVersion) {
        file.close();
        return false;
    }

    const QString projectName = readString(s);
    Q_UNUSED(projectName);

    mgr.reset(int(width), int(height));

    mgr.clearAllLayers();

    for (quint32 i = 0; i < layerCount; ++i) {
        const QString name = readString(s);
        quint8 visible = 1, alphaLock = 0, blend = 0;
        float opacity = 1.0f;
        s >> visible >> alphaLock >> blend >> opacity;

        quint32 pngSize = 0;
        s >> pngSize;

        QByteArray png(int(pngSize), Qt::Uninitialized);
        s.readRawData(png.data(), int(pngSize));

        auto layer = mgr.addLayer(name);
        layer->setVisible(visible != 0);
        layer->setAlphaLocked(alphaLock != 0);
        layer->setBlendMode(static_cast<BlendMode>(blend));
        layer->setOpacity(opacity);
        layer->image() = pngToImage(png);
    }

    if (mgr.layerCount() == 0)
        mgr.addLayer("Background");

    file.close();
    emit mgr.layersChanged();
    return s.status() == QDataStream::Ok;
}
