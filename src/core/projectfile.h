#pragma once

#include <QString>
#include <QDataStream>

class LayerManager;

namespace ProjectFile {

constexpr quint32 kMagic   = 0x00584F4E;
constexpr quint16 kVersion = 1;
constexpr const char* kExtension = "nox";
constexpr const char* kFileFilter = "Projekty NOX (*.nox);;Wszystkie pliki (*)";

bool save(const QString& path, const LayerManager& mgr);
bool load(const QString& path, LayerManager& mgr);

} // namespace ProjectFile
