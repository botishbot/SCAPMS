#include "controller/UndoManager.h"

#include "utils/ClinicException.h"

#include <utility>

namespace scapms::controller {

using utils::ClinicException;
using utils::ErrorCode;

void UndoManager::push(std::unique_ptr<UndoCommand> command) {
    if (!command) return;
    m_descriptions.push(command->description());
    m_stack.push(std::move(command));
    notifyChanged();
}

std::string UndoManager::nextDescription() const {
    return m_stack.empty() ? std::string() : m_stack.top()->description();
}

std::string UndoManager::undoLast() {
    if (m_stack.empty()) throw ClinicException(ErrorCode::UndoUnavailable, "There is nothing to undo.");

    // Pop first: if this command can no longer run it must not block the entries beneath it.
    std::unique_ptr<UndoCommand> command = std::move(m_stack.top());
    m_stack.pop();
    m_descriptions.pop();
    const std::string description = command->description();
    try {
        command->undo(m_database);
    } catch (const ClinicException& e) {
        notifyChanged();
        throw ClinicException(ErrorCode::UndoUnavailable,
                              "Cannot undo \"" + description + "\": " + e.what() +
                                  " This entry was discarded; earlier operations can still be undone.");
    }
    notifyChanged();
    return description;
}

std::vector<std::string> UndoManager::descriptions() const {
    std::vector<std::string> out;
    std::stack<std::string> copy = m_descriptions;  // walk a copy, newest first
    while (!copy.empty()) {
        out.push_back(copy.top());
        copy.pop();
    }
    return out;
}

void UndoManager::clear() {
    while (!m_stack.empty()) m_stack.pop();
    while (!m_descriptions.empty()) m_descriptions.pop();
    notifyChanged();
}

} // namespace scapms::controller
