#include "GameScene.h"
#include "ui/CocosGUI.h"

USING_NS_CC;

Scene* GameScene::createScene() {
    return GameScene::create();
}

bool GameScene::init() {
    if (!Scene::init()) {
        return false;
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();

    // 1. Background (Tan)
    auto layer = LayerColor::create(Color4B(160, 120, 50, 255));
    this->addChild(layer, -1);

    // 2. Bottom area (Purple)
    auto bottomBar = LayerColor::create(Color4B(128, 0, 128, 255), visibleSize.width, 500);
    bottomBar->setPosition(Vec2(0, 0));
    this->addChild(bottomBar, 0);

    _gameModel.setupGame();
    setupView();

    _draggedSprite = nullptr;

    auto listener = EventListenerTouchOneByOne::create();
    listener->onTouchBegan = CC_CALLBACK_2(GameScene::onTouchBegan, this);
    listener->onTouchMoved = CC_CALLBACK_2(GameScene::onTouchMoved, this);
    listener->onTouchEnded = CC_CALLBACK_2(GameScene::onTouchEnded, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    auto undoButton = ui::Button::create();
    undoButton->setTitleText("UNDO");
    undoButton->setTitleFontSize(40);
    undoButton->setPosition(Vec2(visibleSize.width - 150, 200));
    undoButton->addClickEventListener([this](Ref* sender) {
        onUndoClicked();
    });
    this->addChild(undoButton);

    return true;
}

void GameScene::setupView() {
    auto visibleSize = Director::getInstance()->getVisibleSize();
    
    // Clear existing views
    for (auto sprite : _cardSprites) {
        sprite->removeFromParent();
    }
    _cardSprites.clear();
    _modelToSprite.clear();

    auto& mainArea = _gameModel.getMainAreaCards();
    size_t numCols = mainArea.size();

    // Layout parameters for Main Pile
    float gapX = 200.0f; // Significantly increased from 150.0f
    float gapY = 120.0f; // Increased from 85.0f to show most of the card
    float startY = 1500.0f; // Lowered from 1700.0f to center the pile better

    // Calculate dynamic startX to center the entire block
    // Total width occupied by centers = (numCols - 1) * gapX
    // startX is the center of the first column
    float startX = (visibleSize.width - (static_cast<float>(numCols) - 1.0f) * gapX) / 2.0f;

    for (size_t i = 0; i < mainArea.size(); ++i) {
        for (size_t j = 0; j < mainArea[i].size(); ++j) {
            auto model = mainArea[i][j];
            auto sprite = CardSprite::create(model);
            sprite->setPosition(startX + static_cast<float>(i) * gapX, startY - static_cast<float>(j) * gapY);
            this->addChild(sprite);
            
            _cardSprites.push_back(sprite);
            _modelToSprite[model] = sprite;
        }
    }

    // Layout Bottom area (Hand area)
    float handX = visibleSize.width / 2.0f; // Center horizontally
    float handY = 300.0f;
    auto& handCards = _gameModel.getHandAreaCards();
    for (auto model : handCards) {
        auto sprite = CardSprite::create(model);
        sprite->setPosition(handX, handY);
        this->addChild(sprite);
        
        _cardSprites.push_back(sprite);
        _modelToSprite[model] = sprite;
    }

    // NEW: Also create sprites for stock cards so they are in the map
    auto& stockCards = _gameModel.getStockCards();
    for (auto model : stockCards) {
        auto sprite = CardSprite::create(model);
        sprite->setVisible(false);
        this->addChild(sprite);
        
        _cardSprites.push_back(sprite);
        _modelToSprite[model] = sprite;
    }

    _stockPileSprite = Sprite::create("res/card_general.png");
    _stockPileSprite->setColor(Color3B(100, 100, 100));
    _stockPileSprite->setPosition(Vec2(handX - 300.0f, handY)); // Position relative to hand area
    this->addChild(_stockPileSprite);
    
    auto label = Label::createWithSystemFont("STOCK", "Arial", 24);
    label->setPosition(Vec2(_stockPileSprite->getContentSize().width/2, _stockPileSprite->getContentSize().height/2));
    _stockPileSprite->addChild(label);
}

bool GameScene::onTouchBegan(Touch* touch, Event* event) {
    Vec2 touchPos = touch->getLocation();

    if (_stockPileSprite->getBoundingBox().containsPoint(touchPos)) {
        onStockClicked();
        return true;
    }

    for (auto it = _cardSprites.rbegin(); it != _cardSprites.rend(); ++it) {
        auto sprite = *it;
        if (sprite->getBoundingBox().containsPoint(touchPos)) {
            if (sprite->getModel()->isFaceUp) {
                // Check if it's the top-most card in its column
                // (Only the top card can be dragged in this simple version)
                _draggedSprite = sprite;
                _originalPos = sprite->getPosition();
                _draggedSprite->setLocalZOrder(1000); // Bring to front while dragging
                return true;
            }
        }
    }
    return false;
}

void GameScene::onTouchMoved(Touch* touch, Event* event) {
    if (_draggedSprite) {
        _draggedSprite->setPosition(_draggedSprite->getPosition() + touch->getDelta());
    }
}

void GameScene::onTouchEnded(Touch* touch, Event* event) {
    if (!_draggedSprite) return;

    auto handPos = Vec2(540.0f, 400.0f);
    auto topHandCardModel = _gameModel.getTopHandCard();

    bool matched = false;
    if (topHandCardModel) {
        // Check distance to hand area and matching rules
        float dist = _draggedSprite->getPosition().distance(handPos);
        if (dist < 150.0f && _gameModel.canMatch(_draggedSprite->getModel()->rank, topHandCardModel->rank)) {
            matched = true;
        }
    }

    if (matched) {
        // Snap to hand area and update logic
        auto cardToMove = _draggedSprite->getModel();
        _draggedSprite->runAction(Sequence::create(
            MoveTo::create(0.1f, handPos),
            CallFunc::create([this, cardToMove]() {
                _gameModel.moveCardToHandArea(cardToMove);
                refreshAllCards();
            }),
            nullptr
        ));
        _draggedSprite = nullptr; // Clear immediately to avoid multiple triggers
    } else {
        // Return to original position
        auto spriteToReturn = _draggedSprite;
        spriteToReturn->runAction(Sequence::create(
            MoveTo::create(0.2f, _originalPos),
            CallFunc::create([this]() {
                refreshAllCards(); // Restore Z-orders
            }),
            nullptr
        ));
        _draggedSprite = nullptr; // Clear immediately
    }
}

void GameScene::onCardClicked(CardSprite* clickedSprite) {
    auto clickedModel = clickedSprite->getModel();
    auto topHandCardModel = _gameModel.getTopHandCard();

    if (!topHandCardModel) return;

    if (_gameModel.canMatch(clickedModel->rank, topHandCardModel->rank)) {
        auto handPos = Vec2(540.0f, 400.0f);

        auto moveTo = MoveTo::create(0.2f, handPos);
        auto sequence = Sequence::create(moveTo, CallFunc::create([this, clickedModel]() {
            _gameModel.moveCardToHandArea(clickedModel);
            refreshAllCards();
        }), nullptr);

        clickedSprite->runAction(sequence);
        clickedSprite->setLocalZOrder(100);
    } else {
        auto shake = Sequence::create(MoveBy::create(0.05f, Vec2(10, 0)), MoveBy::create(0.05f, Vec2(-20, 0)), MoveBy::create(0.05f, Vec2(10, 0)), nullptr);
        clickedSprite->runAction(shake);
    }
}

void GameScene::onStockClicked() {
    auto cardModel = _gameModel.drawFromStock();
    if (cardModel) {
        auto sprite = _modelToSprite[cardModel];
        sprite->setPosition(_stockPileSprite->getPosition());
        sprite->setVisible(true);
        
        auto handPos = Vec2(540.0f, 400.0f);
        auto moveTo = MoveTo::create(0.2f, handPos);
        sprite->runAction(Sequence::create(moveTo, CallFunc::create([this]() {
            refreshAllCards();
        }), nullptr));
    } else {
        auto shake = Sequence::create(MoveBy::create(0.05f, Vec2(5, 0)), MoveBy::create(0.05f, Vec2(-10, 0)), MoveBy::create(0.05f, Vec2(5, 0)), nullptr);
        _stockPileSprite->runAction(shake);
    }
}

void GameScene::onUndoClicked() {
    if (_gameModel.undo()) {
        refreshAllCards();
    }
}

void GameScene::refreshAllCards() {
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto& mainArea = _gameModel.getMainAreaCards();
    size_t numCols = mainArea.size();

    // Must match setupView parameters
    float gapX = 200.0f;
    float gapY = 120.0f;
    float startY = 1500.0f;
    float startX = (visibleSize.width - (static_cast<float>(numCols) - 1.0f) * gapX) / 2.0f;

    float handX = visibleSize.width / 2.0f;
    float handY = 300.0f;

    // Update Main Pile
    for (size_t i = 0; i < mainArea.size(); ++i) {
        for (size_t j = 0; j < mainArea[i].size(); ++j) {
            auto model = mainArea[i][j];
            if (_modelToSprite.count(model)) {
                auto sprite = _modelToSprite[model];
                sprite->stopAllActions();
                sprite->setPosition(startX + static_cast<float>(i) * gapX, startY - static_cast<float>(j) * gapY);
                sprite->setVisible(true);
                sprite->setLocalZOrder(static_cast<int>(j));
                sprite->updateView();
            }
        }
    }

    // Update Hand area
    auto& handArea = _gameModel.getHandAreaCards();
    for (size_t i = 0; i < handArea.size(); ++i) {
        auto model = handArea[i];
        if (_modelToSprite.count(model)) {
            auto sprite = _modelToSprite[model];
            sprite->stopAllActions();
            sprite->setPosition(handX, handY);
            sprite->setVisible(true);
            sprite->setLocalZOrder(10 + static_cast<int>(i));
            sprite->updateView();
        }
    }

    auto& stockCards = _gameModel.getStockCards();
    for (auto model : stockCards) {
        if (_modelToSprite.count(model)) {
            _modelToSprite[model]->setVisible(false);
        }
    }
    
    _stockPileSprite->setVisible(!stockCards.empty());
    _stockPileSprite->setPosition(Vec2(handX - 300.0f, handY));
}
