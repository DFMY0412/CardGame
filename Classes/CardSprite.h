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

    void setSelected(bool selected);
    bool isSelected() const { return _isSelected; }

    CC_SYNTHESIZE(CardModel*, _model, Model);

protected:
    CardSprite() : _model(nullptr), _isSelected(false), _selectionNode(nullptr) {}
    virtual ~CardSprite() {}
    bool init(CardModel* model);

private:
    bool _isSelected;
    cocos2d::DrawNode* _selectionNode;
};

#endif // __CARD_SPRITE_H__
