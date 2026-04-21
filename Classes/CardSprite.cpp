#include "CardSprite.h"
#include <string>

USING_NS_CC;

CardSprite* CardSprite::create(CardModel* model) {
    CardSprite* sprite = new (std::nothrow) CardSprite();
    if (sprite && sprite->init(model)) {
        sprite->autorelease();
        return sprite;
    }
    CC_SAFE_DELETE(sprite);
    return nullptr;
}

bool CardSprite::init(CardModel* model) {
    if (!Sprite::initWithFile("res/card_general.png")) {
        return false;
    }
    _model = model;
    updateView();
    return true;
}

void CardSprite::updateView() {
    if (!_model) return;

    this->removeAllChildren();
    _selectionNode = nullptr; // Clear dangling pointer after removeAllChildren

    if (_model->isFaceUp) {
        this->setColor(Color3B::WHITE);

        std::string suitName;
        std::string colorName;

        switch (_model->suit) {
            case CardModel::Suit::SPADE:   suitName = "spade";   colorName = "black"; break;
            case CardModel::Suit::CLUB:    suitName = "club";    colorName = "black"; break;
            case CardModel::Suit::HEART:   suitName = "heart";   colorName = "red";   break;
            case CardModel::Suit::DIAMOND: suitName = "diamond"; colorName = "red";   break;
        }

        std::string rankStr;
        if (_model->rank == 1) rankStr = "A";
        else if (_model->rank == 11) rankStr = "J";
        else if (_model->rank == 12) rankStr = "Q";
        else if (_model->rank == 13) rankStr = "K";
        else rankStr = std::to_string(_model->rank);

        Size contentSize = this->getContentSize();

        // 1. Center large number
        auto bigNumPath = "res/number/big_" + colorName + "_" + rankStr + ".png";
        auto bigNum = Sprite::create(bigNumPath);
        if (bigNum) {
            bigNum->setPosition(Vec2(contentSize.width / 2, contentSize.height / 2));
            this->addChild(bigNum);
        }

        // 2. Small number (top-left)
        auto smallNumPath = "res/number/small_" + colorName + "_" + rankStr + ".png";
        auto smallNum = Sprite::create(smallNumPath);
        if (smallNum) {
            smallNum->setAnchorPoint(Vec2(0, 1));
            smallNum->setPosition(Vec2(10, contentSize.height - 10));
            this->addChild(smallNum);

            // 3. Small suit (top-right)
            auto suitPath = "res/suits/" + suitName + ".png";
            auto smallSuit = Sprite::create(suitPath);
            if (smallSuit) {
                smallSuit->setAnchorPoint(Vec2(1, 1));
                smallSuit->setScale(0.5f);
                smallSuit->setPosition(Vec2(contentSize.width - 10, contentSize.height - 10));
                this->addChild(smallSuit);
            }
        }
    } else {
        this->setColor(Color3B(100, 100, 100));
    }

    // Re-draw selection highlight if needed
    if (_isSelected) {
        setSelected(true);
    }
}

void CardSprite::setSelected(bool selected) {
    _isSelected = selected;
    if (_selectionNode) {
        _selectionNode->removeFromParent();
        _selectionNode = nullptr;
    }

    if (_isSelected) {
        _selectionNode = DrawNode::create();
        float margin = 2.0f;
        Size size = this->getContentSize();
        Vec2 vertices[] = {
            Vec2(-margin, -margin),
            Vec2(size.width + margin, -margin),
            Vec2(size.width + margin, size.height + margin),
            Vec2(-margin, size.height + margin)
        };
        _selectionNode->drawPolygon(vertices, 4, Color4F(1, 0.84f, 0, 0.5f), 3.0f, Color4F(1, 0.84f, 0, 1)); // Gold
        this->addChild(_selectionNode, -1);
    }
}
