#pragma once

#include "command.h"
#include <QObject>
#include <vector>
#include <memory>

class CommandManager : public QObject
{
    Q_OBJECT
public:
    explicit CommandManager(QObject* parent = nullptr) : QObject(parent) {}

    void push(std::unique_ptr<Command> cmd) {
        cmd->redo();
        m_undoStack.push_back(std::move(cmd));
        m_redoStack.clear();
        emit stackChanged();
    }

    void undo() {
        if (m_undoStack.empty()) return;
        auto cmd = std::move(m_undoStack.back());
        m_undoStack.pop_back();
        cmd->undo();
        m_redoStack.push_back(std::move(cmd));
        emit stackChanged();
    }

    void redo() {
        if (m_redoStack.empty()) return;
        auto cmd = std::move(m_redoStack.back());
        m_redoStack.pop_back();
        cmd->redo();
        m_undoStack.push_back(std::move(cmd));
        emit stackChanged();
    }

    void clear() {
        m_undoStack.clear();
        m_redoStack.clear();
        emit stackChanged();
    }

    bool canUndo() const { return !m_undoStack.empty(); }
    bool canRedo() const { return !m_redoStack.empty(); }

    QString undoName() const {
        return m_undoStack.empty() ? QString() : m_undoStack.back()->name();
    }
    QString redoName() const {
        return m_redoStack.empty() ? QString() : m_redoStack.back()->name();
    }

signals:
    void stackChanged();

private:
    std::vector<std::unique_ptr<Command>> m_undoStack;
    std::vector<std::unique_ptr<Command>> m_redoStack;
};
