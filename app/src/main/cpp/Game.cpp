#include "Game.h"
#include <cmath>
#include <cstdlib>
#include <ctime>

Game::Game(int width, int height) : gridWidth_(width), gridHeight_(height) {
    std::srand(std::time(nullptr));
    reset();
}

void Game::reset() {
    level_ = 1;
    generateLevel(level_);
}

void Game::nextLevel() {
    level_++;
    generateLevel(level_);
}

void Game::generateLevel(int lvl) {
    arrows_.clear();
    remainingAttempts_ = 10 + (lvl / 2);
    int size = 3 + (lvl / 5);
    if (size > 6) size = 6;
    gridWidth_ = size;
    gridHeight_ = size;

    for (int y = 0; y < gridHeight_; ++y) {
        for (int x = 0; x < gridWidth_; ++x) {
            Direction d = static_cast<Direction>(std::rand() % 4);
            arrows_.push_back({x, y, d, false, false, (float)x, (float)y, false});
        }
    }
}

void Game::update(float deltaTime) {
    const float speed = 10.0f;
    for (auto& arrow : arrows_) {
        if (arrow.moving && !arrow.gone) {
            if (arrow.dir == Direction::UP) arrow.currentY += speed * deltaTime;
            else if (arrow.dir == Direction::DOWN) arrow.currentY -= speed * deltaTime;
            else if (arrow.dir == Direction::LEFT) arrow.currentX -= speed * deltaTime;
            else if (arrow.dir == Direction::RIGHT) arrow.currentX += speed * deltaTime;

            if (std::abs(arrow.currentX - arrow.x) > 10.0f || std::abs(arrow.currentY - arrow.y) > 10.0f) {
                arrow.gone = true;
                arrow.moving = false;
            }
        }
    }
}

bool Game::isPathClear(const Arrow& arrow) const {
    for (const auto& other : arrows_) {
        if (other.gone || &other == &arrow) continue;
        if (arrow.dir == Direction::UP && other.x == arrow.x && other.y > arrow.y) return false;
        if (arrow.dir == Direction::DOWN && other.x == arrow.x && other.y < arrow.y) return false;
        if (arrow.dir == Direction::LEFT && other.y == arrow.y && other.x < arrow.x) return false;
        if (arrow.dir == Direction::RIGHT && other.y == arrow.y && other.x > arrow.x) return false;
    }
    return true;
}

void Game::giveHint() {
    for (auto& arrow : arrows_) {
        arrow.isHinted = false;
    }
    for (auto& arrow : arrows_) {
        if (!arrow.gone && !arrow.moving && isPathClear(arrow)) {
            arrow.isHinted = true;
            break;
        }
    }
}

bool Game::handleTap(float screenX, float screenY, int screenWidth, int screenHeight) {
    if (isGameOver()) return false;

    float aspect = (float)screenWidth / screenHeight;
    float worldY = (1.0f - (screenY / screenHeight) * 2.0f) * 2.0f;
    float worldX = ((screenX / screenWidth) * 2.0f - 1.0f) * 2.0f * aspect;

    float cellSize = 0.6f;
    float gridOffsetX = (gridWidth_ * cellSize) / 2.0f - (cellSize / 2.0f);
    float gridOffsetY = (gridHeight_ * cellSize) / 2.0f - (cellSize / 2.0f);

    for (auto& arrow : arrows_) {
        arrow.isHinted = false; // مسح التلميح عند الضغط
        if (arrow.gone || arrow.moving) continue;

        float arrowWorldX = (arrow.x * cellSize) - gridOffsetX;
        float arrowWorldY = (arrow.y * cellSize) - gridOffsetY;

        if (std::abs(worldX - arrowWorldX) < 0.25f && std::abs(worldY - arrowWorldY) < 0.25f) {
            if (isPathClear(arrow)) {
                arrow.moving = true;
                return true;
            } else {
                remainingAttempts_--;
                return false;
            }
        }
    }
    return false;
}

bool Game::isLevelCleared() const {
    for (const auto& arrow : arrows_) {
        if (!arrow.gone) return false;
    }
    return true;
}
