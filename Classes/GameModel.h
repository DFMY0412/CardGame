#ifndef __GAME_MODEL_H__
#define __GAME_MODEL_H__

#include <vector>
#include <stack>
#include "CardModel.h"

/**
 * @struct GameState
 * @brief Snapshot of the game state for undo/redo.
 */
struct GameState {
    std::vector<std::vector<CardModel*>> mainAreaCards;
    std::vector<CardModel*> handAreaCards;
    std::vector<CardModel*> stockCards;
    std::vector<bool> faceUpStatus;
};

/**
 * @class GameModel
 * @brief Manages the logic and state of the card game.
 */
class GameModel {
public:
    GameModel();
    virtual ~GameModel();

    void setupGame();
    void saveState();
    bool undo();
    CardModel* drawFromStock();

    bool canMatch(int rank1, int rank2);
    void moveCardToHandArea(CardModel* card);

    const std::vector<std::vector<CardModel*>>& getMainAreaCards() const { return _mainAreaCards; }
    const std::vector<CardModel*>& getHandAreaCards() const { return _handAreaCards; }
    const std::vector<CardModel*>& getStockCards() const { return _stockCards; }

    CardModel* getTopHandCard() const;
    void updateFaceUpStatus();

    /**
     * @brief Check if game is won (Tableau is empty)
     */
    bool checkWin() const;

    /**
     * @brief Check if game is over (No legal moves and Stock is empty)
     */
    bool checkGameOver();

private:
    std::vector<std::vector<CardModel*>> _mainAreaCards;
    std::vector<CardModel*> _handAreaCards;
    std::vector<CardModel*> _stockCards;
    std::vector<CardModel*> _allCards;
    std::stack<GameState> _history;
};

#endif // __GAME_MODEL_H__
