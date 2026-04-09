#pragma once
#include <string>

class CommandBlockEntity;

class IUIScene_CommandBlockMenu {
public:
    virtual ~IUIScene_CommandBlockMenu() = default;
    void Initialise(CommandBlockEntity* commandBlock);

protected:
    void ConfirmButtonClicked();

    virtual std::string GetCommand() = 0;
    virtual void SetCommand(std::string command) = 0;
    virtual int GetPad() = 0;

private:
    CommandBlockEntity* m_commandBlock;
};
