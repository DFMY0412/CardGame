#ifndef __GAME_SCENE_H__
#define __GAME_SCENE_H__

#include "cocos2d.h"
#include "GameModel.h"
#include "CardSprite.h"
#include <vector>
#include <map>

/**
 * @class GameScene
 * @brief Controller and View for the card game.
 */
class GameScene : public cocos2d::Scene {
public:
    static cocos2d::Scene* createScene();
    virtual bool init() override;
    CREATE_FUNC(GameScene);

    void setupView();
    
    bool onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchMoved(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchEnded(cocos2d::Touch* touch, cocos2d::Event* event);

    void onCardClicked(CardSprite* sprite);
    void onStockClicked();
    void onUndoClicked();

    void refreshAllCards();

private:
    GameModel _gameModel;
    std::vector<CardSprite*> _cardSprites;
    std::map<CardModel*, CardSprite*> _modelToSprite;
    cocos2d::Sprite* _stockPileSprite;

    // Drag and Drop support
    CardSprite* _draggedSprite;
    cocos2d::Vec2 _originalPos;
};

#endif // __GAME_SCENE_H__
