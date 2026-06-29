#pragma once

#include <unordered_map>
#include <variant>
#include <vector>

#include "InputAction.h"
#include "InputCodes.h"

namespace voxl
{
    using InputBinding = std::variant<KeyCode, MouseCode>;

    class ActionManager
    {
    public:
        ActionManager() = default;
        ~ActionManager() = default;

        ActionManager(const ActionManager&) = delete;
        ActionManager& operator=(const ActionManager&) = delete;
        ActionManager(ActionManager&&) noexcept = default;
        ActionManager& operator=(ActionManager&&) noexcept = default;

        void bindAction(InputAction action, InputBinding binding);
        void clearAction(InputAction action);
        
        [[nodiscard]] bool isActionPressed(InputAction action) const;

    private:
        std::unordered_map<InputAction, std::vector<InputBinding>> m_actionMap;
    };
} // namespace voxl
