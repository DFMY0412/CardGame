#ifndef __CARD_MODEL_H__
#define __CARD_MODEL_H__

/**
 * @class CardModel
 * @brief Represents data for a single playing card.
 */
class CardModel {
public:
    enum class Suit {
        SPADE,
        HEART,
        CLUB,
        DIAMOND
    };

    CardModel(int rank, Suit suit, bool isFaceUp = false)
        : rank(rank), suit(suit), isFaceUp(isFaceUp) {}

    int rank;
    Suit suit;
    bool isFaceUp;
};

#endif // __CARD_MODEL_H__
