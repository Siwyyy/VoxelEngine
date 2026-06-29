#include "ActionManager.h"

#include <algorithm>

#include "Input.h"

namespace voxl
{
    void ActionManager::bindAction(InputAction action, InputBinding binding)
    {
        m_actionMap[action].push_back(binding);
    }

    void ActionManager::clearAction(InputAction action)
    {
        m_actionMap.erase(action);
    }

    bool ActionManager::isActionPressed(InputAction action) const
    {
        const auto it = m_actionMap.find(action);

        if (it == m_actionMap.end())
        {
            return false;
        }

        return std::ranges::any_of(it->second, [](const InputBinding& binding)
        {
            if (std::holds_alternative<KeyCode>(binding))
            {
                return Input::isKeyPressed(std::get<KeyCode>(binding));
            }
            if (std::holds_alternative<MouseCode>(binding))
            {
                return Input::isMouseButtonPressed(std::get<MouseCode>(binding));
            }
            return false;
        });
    }
} // namespace voxl
