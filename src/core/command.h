#pragma once

#include <QImage>
#include <QString>
#include <functional>
#include <utility>

// Command
class Command {
public:
    virtual ~Command() = default;
    virtual void undo() = 0;
    virtual void redo() = 0;
    virtual QString name() const { return QStringLiteral("Akcja"); }
};

// DrawCommand
class DrawCommand : public Command {
public:
    DrawCommand(QImage* target, QImage before, QImage after,
                QString name = QStringLiteral("Rysowanie"))
        : m_target(target)
        , m_before(std::move(before))
        , m_after(std::move(after))
        , m_name(std::move(name))
    {}

    void undo() override { if (m_target) *m_target = m_before; }
    void redo() override { if (m_target) *m_target = m_after;  }
    QString name() const override { return m_name; }

private:
    QImage* m_target = nullptr;
    QImage  m_before;
    QImage  m_after;
    QString m_name;
};

// FilterCommand
class FilterCommand : public Command {
public:
    using FilterFn = std::function<QImage(const QImage&)>;

    FilterCommand(QImage* target,
                  QImage before,
                  FilterFn filter,
                  QString name)
        : m_target(target)
        , m_before(std::move(before))
        , m_filter(std::move(filter))
        , m_name(std::move(name))
    {}

    void undo() override {
        if (m_target) *m_target = m_before;
    }

    void redo() override {
        if (!m_afterComputed) {
            m_after = m_filter(m_before);
            m_afterComputed = true;
        }
        if (m_target) *m_target = m_after;
    }

    QString name() const override { return m_name; }

private:
    QImage*  m_target = nullptr;
    QImage   m_before;
    QImage   m_after;
    bool     m_afterComputed = false;
    FilterFn m_filter;
    QString  m_name;
};
