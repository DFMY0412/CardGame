#include "GameModel.h"
#include <algorithm>
#include <random>
#include <ctime>

GameModel::GameModel() {
}

GameModel::~GameModel() {
    for (auto card : _allCards) {
        delete card;
    }
    _allCards.clear();
}

void GameModel::setupGame() {
    for (auto card : _allCards) {
        delete card;
    }
    _allCards.clear();
    _mainAreaCards.clear();
    _handAreaCards.clear();
    _stockCards.clear();
    while(!_history.empty()) _history.pop();

    std::vector<CardModel*> deck;
    for (int suit = 0; suit < 4; ++suit) {
        for (int rank = 1; rank <= 13; ++rank) {
            auto card = new CardModel(rank, static_cast<CardModel::Suit>(suit), false);
            deck.push_back(card);
            _allCards.push_back(card);
        }
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(deck.begin(), deck.end(), g);

    // 4. Setup Main Pile (7 columns * 4 cards = 28 cards)
    _mainAreaCards.resize(7);
    int deckIndex = 0;
    for (int i = 0; i < 7; ++i) {
        // Each column gets 4 cards
        for (int j = 0; j < 4; ++j) {
            CardModel* card = deck[deckIndex++];
            if (j == 3) card->isFaceUp = true;
            _mainAreaCards[i].push_back(card);
        }
    }

    CardModel* firstHandCard = deck[deckIndex++];
    firstHandCard->isFaceUp = true;
    _handAreaCards.push_back(firstHandCard);

    while (static_cast<size_t>(deckIndex) < deck.size()) {
        _stockCards.push_back(deck[deckIndex++]);
    }
}

void GameModel::saveState() {
    GameState state;
    state.mainAreaCards = _mainAreaCards;
    state.handAreaCards = _handAreaCards;
    state.stockCards = _stockCards;
    
    for (auto card : _allCards) {
        state.faceUpStatus.push_back(card->isFaceUp);
    }
    
    _history.push(state);
}

bool GameModel::undo() {
    if (_history.empty()) return false;

    GameState lastState = _history.top();
    _history.pop();

    _mainAreaCards = lastState.mainAreaCards;
    _handAreaCards = lastState.handAreaCards;
    _stockCards = lastState.stockCards;

    for (size_t i = 0; i < _allCards.size(); ++i) {
        _allCards[i]->isFaceUp = lastState.faceUpStatus[i];
    }

    return true;
}

CardModel* GameModel::drawFromStock() {
    if (_stockCards.empty()) return nullptr;

    saveState();

    CardModel* card = _stockCards.back();
    _stockCards.pop_back();
    card->isFaceUp = true;
    _handAreaCards.push_back(card);
    
    return card;
}

bool GameModel::canMatch(int rank1, int rank2) {
    int diff = std::abs(rank1 - rank2);
    return diff == 1 || diff == 12;
}

void GameModel::moveCardToHandArea(CardModel* card) {
    saveState();

    for (auto& column : _mainAreaCards) {
        auto it = std::find(column.begin(), column.end(), card);
        if (it != column.end()) {
            column.erase(it);
            break;
        }
    }
    card->isFaceUp = true;
    _handAreaCards.push_back(card);
    
    updateFaceUpStatus();
}

CardModel* GameModel::getTopHandCard() const {
    if (_handAreaCards.empty()) return nullptr;
    return _handAreaCards.back();
}

void GameModel::updateFaceUpStatus() {
    for (auto& column : _mainAreaCards) {
        if (!column.empty()) {
            column.back()->isFaceUp = true;
        }
    }
}

bool GameModel::checkWin() const {
    for (const auto& column : _mainAreaCards) {
        if (!column.empty()) return false;
    }
    return true;
}

bool GameModel::checkGameOver() {
    // 1. If stock is not empty, game is not over
    if (!_stockCards.empty()) return false;

    // 2. If already won, it's not "Game Over" (failure)
    if (checkWin()) return false;

    CardModel* topHand = getTopHandCard();
    if (!topHand) return false;

    // 3. Check if any card in main area (tableau) can match the top hand card
    for (const auto& column : _mainAreaCards) {
        if (!column.empty()) {
            if (canMatch(column.back()->rank, topHand->rank)) {
                return false;
            }
        }
    }

    // 4. If we reached here, no moves are possible and stock is empty
    return true;
}
