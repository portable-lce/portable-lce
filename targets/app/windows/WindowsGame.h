#pragma once

class WindowsGame : public Game {
public:
    WindowsGame();

    virtual void StoreLaunchData();
    virtual void ExitGame();
    virtual void FatalLoadError();

    C4JStringTable* GetStringTable() { return nullptr; }

    // original code
    virtual void TemporaryCreateGameStart();
};

extern WindowsGame app;
