#pragma once
#include "minecraft/client/gui/Screen.h"
#include "minecraft/stats/Achievements.h"

class StatsCounter;

class AchievementScreen : public Screen {
private:
    static const int BIGMAP_X = 16;
    static const int BIGMAP_Y = 17;
    static const int BIGMAP_WIDTH = 224;
    static const int BIGMAP_HEIGHT = 155;

    // number of pixels per achievement
    static const int ACHIEVEMENT_COORD_SCALE = 24;
    static const int EDGE_VALUE_X =
        Achievements::ACHIEVEMENT_WIDTH_POSITION * ACHIEVEMENT_COORD_SCALE;
    static const int EDGE_VALUE_Y =
        Achievements::ACHIEVEMENT_HEIGHT_POSITION * ACHIEVEMENT_COORD_SCALE;

    int xMin;
    int yMin;
    int xMax;
    int yMax;

    static const int MAX_BG_TILE_Y = (EDGE_VALUE_Y * 2 - 1) / 16;

protected:
    int imageWidth;
    int imageHeight;
    int xLastScroll;
    int yLastScroll;

protected:
    double xScrollO, yScrollO;
    double xScrollP, yScrollP;
    double xScrollTarget, yScrollTarget;

private:
    int scrolling;
    StatsCounter* statsCounter;

public:
    using Screen::keyPressed;

    AchievementScreen(StatsCounter* statsCounter);
    virtual void init() override;

protected:
    virtual void buttonClicked(Button* button) override;
    virtual void keyPressed(char eventCharacter, int eventKey) override;

public:
    virtual void render(int mouseX, int mouseY, float a) override;
    virtual void tick() override;

protected:
    virtual void renderLabels();
    virtual void renderBg(int xm, int ym, float a);

public:
    virtual bool isPauseScreen() override;
};
