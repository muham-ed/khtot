#ifndef KHTOT_GAME_H
#define KHTOT_GAME_H

#include <vector>
#include <memory>

enum class Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

struct Arrow {
    int x;
    int y;
    Direction dir;
    bool moving = false;
    bool gone = false;
    float currentX;
    float currentY;
};

class Game {
public:
    Game(int width, int height);
    void reset();
    void update(float deltaTime);
    bool handleTap(float screenX, float screenY, int screenWidth, int screenHeight);

    const std::vector<Arrow>& getArrows() const { return arrows_; }
    int getRemainingAttempts() const { return remainingAttempts_; }
    bool isLevelCleared() const;
    void nextLevel();
    int getLevel() const { return level_; }

private:
    int gridWidth_;
    int gridHeight_;
    std::vector<Arrow> arrows_;
    int remainingAttempts_;
    int level_ = 1;

    bool isPathClear(const Arrow& arrow) const;
    void generateLevel(int lvl);
};

#endif //KHTOT_GAME_H
