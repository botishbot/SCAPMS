#pragma once
#include "model/ClinicDatabase.h"

#include <functional>
#include <memory>
#include <stack>
#include <string>
#include <vector>

namespace scapms::controller {

// One reversible administrative operation (command pattern).
// undo() must be all-or-nothing: check everything first, then change data.
class UndoCommand {
public:
    virtual ~UndoCommand() = default;
    virtual std::string description() const = 0;            // e.g. "Delete patient P-0007 (Ada Obi)"
    virtual void undo(model::ClinicDatabase& database) = 0;  // throws ClinicException(UndoUnavailable)
};

// "Undo Last Administrative Operation" built on std::stack (last in, first out).
class UndoManager {
public:
    explicit UndoManager(model::ClinicDatabase& database) : m_database(database) {}

    void push(std::unique_ptr<UndoCommand> command);
    bool canUndo() const { return !m_stack.empty(); }
    std::size_t size() const { return m_stack.size(); }
    std::string nextDescription() const;
    std::vector<std::string> descriptions() const;  // every undoable operation, newest first  // description of what Undo would reverse ("" if nothing)

    // Reverses the most recent operation and returns its description.
    // If it cannot be reversed any more, the entry is discarded and ClinicException(UndoUnavailable)
    // explains why (the older entries stay available).
    std::string undoLast();

    void clear();  // used by New and Load
    void setChangedCallback(std::function<void()> callback) { m_onChanged = std::move(callback); }

private:
    void notifyChanged() { if (m_onChanged) m_onChanged(); }

    model::ClinicDatabase& m_database;
    std::stack<std::unique_ptr<UndoCommand>> m_stack;
    std::stack<std::string> m_descriptions;  // same order as m_stack, so the list can be shown
    std::function<void()> m_onChanged;
};

} // namespace scapms::controller
