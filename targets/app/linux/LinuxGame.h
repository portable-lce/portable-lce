#pragma once

#include "app/common/Game.h"

class C4JStringTable;

class LinuxGame : public Game {
public:
    LinuxGame();

    void StoreLaunchData() override;
    void ExitGame() override;
    void FatalLoadError() override;

    C4JStringTable* GetStringTable() { return nullptr; }

    // original code
    virtual void TemporaryCreateGameStart();
};

extern LinuxGame app;

