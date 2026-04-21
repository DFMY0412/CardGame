#ifndef __CARD_SPRITE_H__
#define __CARD_SPRITE_H__

#include "cocos2d.h"
#include "CardModel.h"

/**
 * @class CardSprite
 * @brief Visual representation of a card.
 */
class CardSprite : public cocos2d::Sprite {
public:
    static CardSprite* create(CardModel* model);
    void updateView();

    CC_SYNTHESIZE(CardModel*, _model, Model);

protected:
    CardSprite() : _model(nullptr) {}
    virtual ~CardSprite() {}
    bool init(CardModel* model);
};

#endif // __CARD_SPRITE_H__
