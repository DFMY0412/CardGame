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

    _selectedSprite = nullptr;
    _draggedSprite = nullptr;
    _stockPileSprite = nullptr;
    _stockCountLabel = nullptr;
    _progressLabel = nullptr;

    setupView();
    
    // Add Event Listener
    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
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

float GameScene::getCardScale() {
    auto visibleSize = Director::getInstance()->getVisibleSize();
    // Target width is 13% of screen width (between 12.5% and 14%)
    float targetCardWidth = visibleSize.width * 0.13f;
    
    // Original card texture width is 140 (based on res/card_general.png)
    // We use a temporary sprite to get the actual content size
    float originalWidth = 140.0f; 
    return targetCardWidth / originalWidth;
}

void GameScene::setupView() {
    auto visibleSize = Director::getInstance()->getVisibleSize();
    float cardScale = getCardScale();
    
    // Clear existing views
    for (auto sprite : _cardSprites) {
        sprite->removeFromParent();
    }
    _cardSprites.clear();
    _modelToSprite.clear();

    auto& mainArea = _gameModel.getMainAreaCards();
    size_t numCols = mainArea.size();

    // --- Optimized Dynamic Layout for 7 Columns ---
    float sideMargin = visibleSize.width * 0.05f; // 5% margin
    float availableWidth = visibleSize.width - (sideMargin * 2.0f);
    
    // gapX is the distance between column centers
    float gapX = availableWidth / static_cast<float>(numCols); 
    float startX = sideMargin + (gapX / 2.0f);

    // gapY based on scaled card height (original height ~190)
    float gapY = (190.0f * cardScale) * 0.45f; 
    float startY = visibleSize.height * 0.75f;

    for (size_t i = 0; i < mainArea.size(); ++i) {
        for (size_t j = 0; j < mainArea[i].size(); ++j) {
            auto model = mainArea[i][j];
            auto sprite = CardSprite::create(model);
            sprite->setScale(cardScale);
            sprite->setPosition(startX + static_cast<float>(i) * gapX, startY - static_cast<float>(j) * gapY);
            this->addChild(sprite);
            
            _cardSprites.push_back(sprite);
            _modelToSprite[model] = sprite;
        }
    }

    // Layout Bottom area (Hand area)
    float handX = visibleSize.width / 2.0f; 
    float handY = visibleSize.height * 0.15f;
    auto& handCards = _gameModel.getHandAreaCards();
    for (auto model : handCards) {
        auto sprite = CardSprite::create(model);
        sprite->setScale(cardScale);
        sprite->setPosition(handX, handY);
        this->addChild(sprite);
        
        _cardSprites.push_back(sprite);
        _modelToSprite[model] = sprite;
    }

    // Stock Pile
    auto& stockCards = _gameModel.getStockCards();
    for (auto model : stockCards) {
        auto sprite = CardSprite::create(model);
        sprite->setScale(cardScale);
        sprite->setVisible(false);
        this->addChild(sprite);
        
        _cardSprites.push_back(sprite);
        _modelToSprite[model] = sprite;
    }

    // Initialize or update Static UI Elements
    if (!_stockPileSprite) {
        _stockPileSprite = Sprite::create("res/card_general.png");
        this->addChild(_stockPileSprite);
        
        _stockCountLabel = Label::createWithSystemFont("", "Arial", 40);
        _stockCountLabel->setPosition(Vec2(_stockPileSprite->getContentSize().width/2, _stockPileSprite->getContentSize().height/2));
        _stockCountLabel->setTextColor(Color4B::WHITE);
        _stockCountLabel->enableOutline(Color4B::BLACK, 2);
        _stockPileSprite->addChild(_stockCountLabel);
    }
    _stockPileSprite->setScale(cardScale);
    _stockPileSprite->setColor(Color3B(100, 100, 100));
    _stockPileSprite->setPosition(Vec2(handX - (visibleSize.width * 0.25f), handY));

    if (!_progressLabel) {
        _progressLabel = Label::createWithSystemFont("", "Arial", 32);
        _progressLabel->setAnchorPoint(Vec2(1, 1));
        _progressLabel->setTextColor(Color4B::WHITE);
        _progressLabel->enableOutline(Color4B::BLACK, 2);
        this->addChild(_progressLabel);
    }
    _progressLabel->setPosition(Vec2(visibleSize.width - 20, visibleSize.height - 20));

    refreshAllCards();
}

bool GameScene::onTouchBegan(Touch* touch, Event* event) {
    Vec2 touchPos = touch->getLocation();

    // 1. Check Stock Pile Click
    if (_stockPileSprite->getBoundingBox().containsPoint(touchPos)) {
        onStockClicked();
        if (_selectedSprite) {
            _selectedSprite->setSelected(false);
            _selectedSprite = nullptr;
        }
        return true;
    }

    // 2. Check Card Clicks (Tableau)
    for (auto it = _cardSprites.rbegin(); it != _cardSprites.rend(); ++it) {
        auto sprite = *it;
        if (sprite->getBoundingBox().containsPoint(touchPos)) {
            if (sprite->getModel()->isFaceUp) {
                // Perform Match Logic on Click
                onCardClicked(sprite);
                
                // Visual Selection Feedback
                if (_selectedSprite) {
                    _selectedSprite->setSelected(false);
                }
                _selectedSprite = sprite;
                _selectedSprite->setSelected(true);
                
                return true;
            }
        }
    }

    // 3. Clicked empty space: clear selection
    if (_selectedSprite) {
        _selectedSprite->setSelected(false);
        _selectedSprite = nullptr;
    }

    return false;
}

void GameScene::onTouchMoved(Touch* touch, Event* event) {
    // Dragging disabled
}

void GameScene::onTouchEnded(Touch* touch, Event* event) {
    // Logic moved to onTouchBegan (Click)
}

void GameScene::showGameResult(bool isWin) {
    auto visibleSize = Director::getInstance()->getVisibleSize();

    // 1. Semi-transparent background
    auto overlay = LayerColor::create(Color4B(0, 0, 0, 180));
    overlay->setName("result_layer");
    this->addChild(overlay, 2000);

    // 2. Result Label
    std::string message = isWin ? "CONGRATULATIONS!" : "NO MORE MOVES";
    auto label = Label::createWithSystemFont(message, "Arial", 80);
    label->setPosition(Vec2(visibleSize.width / 2, visibleSize.height / 2 + 100));
    label->setTextColor(isWin ? Color4B::YELLOW : Color4B::WHITE);
    label->enableOutline(Color4B::BLACK, 3);
    overlay->addChild(label);

    // 3. Restart Button
    auto restartButton = ui::Button::create();
    restartButton->setTitleText("RESTART");
    restartButton->setTitleFontSize(50);
    restartButton->setTitleColor(Color3B::WHITE);
    restartButton->setPosition(Vec2(visibleSize.width / 2, visibleSize.height / 2 - 100));
    restartButton->addClickEventListener([this](Ref* sender) {
        resetGame();
    });
    overlay->addChild(restartButton);

    // 4. Disable interactions on game scene
    // Delay listener removal to next frame to avoid crash in current event dispatch
    this->scheduleOnce([this](float dt) {
        _eventDispatcher->removeEventListenersForTarget(this);
    }, 0, "remove_listeners_key");
}

void GameScene::onCardClicked(CardSprite* clickedSprite) {
    auto clickedModel = clickedSprite->getModel();
    auto topHandCardModel = _gameModel.getTopHandCard();

    if (!topHandCardModel) return;

    if (_gameModel.canMatch(clickedModel->rank, topHandCardModel->rank)) {
        auto visibleSize = Director::getInstance()->getVisibleSize();
        auto handPos = Vec2(visibleSize.width / 2.0f, visibleSize.height * 0.15f);

        clickedSprite->setLocalZOrder(1000);
        auto moveTo = MoveTo::create(0.15f, handPos);
        auto sequence = Sequence::create(moveTo, CallFunc::create([this, clickedModel]() {
            _gameModel.moveCardToHandArea(clickedModel);
            refreshAllCards();

            // Check Win/GameOver after move
            if (_gameModel.checkWin()) {
                showGameResult(true);
            } else if (_gameModel.checkGameOver()) {
                showGameResult(false);
            }
        }), nullptr);

        clickedSprite->runAction(sequence);
    } else {
        // Shake animation for invalid match
        auto shake = Sequence::create(
            MoveBy::create(0.05f, Vec2(10, 0)), 
            MoveBy::create(0.05f, Vec2(-20, 0)), 
            MoveBy::create(0.05f, Vec2(10, 0)), 
            nullptr
        );
        clickedSprite->runAction(shake);
    }
}

void GameScene::onStockClicked() {
    auto cardModel = _gameModel.drawFromStock();
    if (cardModel) {
        auto sprite = _modelToSprite[cardModel];
        sprite->stopAllActions();
        sprite->setPosition(_stockPileSprite->getPosition());
        sprite->setVisible(true);
        sprite->setLocalZOrder(2000); // Topmost during move
        
        auto visibleSize = Director::getInstance()->getVisibleSize();
        auto handPos = Vec2(visibleSize.width / 2.0f, visibleSize.height * 0.15f);
        
        auto moveTo = MoveTo::create(0.2f, handPos);
        sprite->runAction(Sequence::create(moveTo, CallFunc::create([this]() {
            refreshAllCards();

            // Check GameOver after drawing from stock
            if (_gameModel.checkGameOver()) {
                showGameResult(false);
            }
        }), nullptr));
    } else {
        auto shake = Sequence::create(MoveBy::create(0.05f, Vec2(5, 0)), MoveBy::create(0.05f, Vec2(-10, 0)), MoveBy::create(0.05f, Vec2(5, 0)), nullptr);
        _stockPileSprite->runAction(shake);
    }
}

void GameScene::resetGame() {
    // 1. Reset Model
    _gameModel.setupGame();

    // 2. Clear state
    _selectedSprite = nullptr;
    _draggedSprite = nullptr;

    // 3. Reset View
    setupView();

    // 4. Re-enable input if it was disabled
    _eventDispatcher->removeEventListenersForTarget(this);
    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = CC_CALLBACK_2(GameScene::onTouchBegan, this);
    listener->onTouchMoved = CC_CALLBACK_2(GameScene::onTouchMoved, this);
    listener->onTouchEnded = CC_CALLBACK_2(GameScene::onTouchEnded, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    // 5. Remove any result overlays
    auto resultLayer = this->getChildByName("result_layer");
    if (resultLayer) {
        resultLayer->removeFromParent();
    }
}

void GameScene::onUndoClicked() {
    // 1. Snapshot current positions of all sprites before logic undo
    std::map<CardModel*, Vec2> currentPositions;
    for (auto const& pair : _modelToSprite) {
        currentPositions[pair.first] = pair.second->getPosition();
    }

    // 2. Perform logic undo
    if (_gameModel.undo()) {
        // 3. After undo, identify which cards have "moved back"
        // We need to refresh logical positions first but NOT stop actions immediately
        refreshAllCards(); 

        // 4. Apply back-translation animation for all cards
        for (auto const& pair : _modelToSprite) {
            auto model = pair.first;
            auto sprite = pair.second;
            Vec2 targetPos = sprite->getPosition(); // This is the new (original) position after refresh
            Vec2 oldPos = currentPositions[model];  // This was where it was just a moment ago
            
            if (oldPos.distance(targetPos) > 1.0f) {
                sprite->setPosition(oldPos);
                sprite->setLocalZOrder(2000); // Ensure it's on top during return
                sprite->runAction(Sequence::create(
                    MoveTo::create(0.2f, targetPos),
                    CallFunc::create([this]() {
                        // Optional: finalize Z-order or state
                    }),
                    nullptr
                ));
            }
        }
    }
}

Vec2 GameScene::getCardPosition(CardModel* model) {
    auto visibleSize = Director::getInstance()->getVisibleSize();
    float cardScale = getCardScale();

    // Re-calculate layout constants (same as in refreshAllCards)
    auto& mainArea = _gameModel.getMainAreaCards();
    size_t numCols = mainArea.size();
    float sideMargin = visibleSize.width * 0.05f; 
    float availableWidth = visibleSize.width - (sideMargin * 2.0f);
    float gapX = availableWidth / static_cast<float>(numCols); 
    float startX = sideMargin + (gapX / 2.0f);
    float gapY = (190.0f * cardScale) * 0.45f;
    float startY = visibleSize.height * 0.75f;

    float handX = visibleSize.width / 2.0f;
    float handY = visibleSize.height * 0.15f;

    // Search in Main Pile
    for (size_t i = 0; i < mainArea.size(); ++i) {
        for (size_t j = 0; j < mainArea[i].size(); ++j) {
            if (mainArea[i][j] == model) {
                return Vec2(startX + static_cast<float>(i) * gapX, startY - static_cast<float>(j) * gapY);
            }
        }
    }

    // Search in Hand Area
    auto& handArea = _gameModel.getHandAreaCards();
    for (size_t i = 0; i < handArea.size(); ++i) {
        if (handArea[i] == model) {
            return Vec2(handX, handY);
        }
    }

    // Search in Stock (invisible)
    auto& stockCards = _gameModel.getStockCards();
    for (auto m : stockCards) {
        if (m == model) {
            return _stockPileSprite->getPosition();
        }
    }

    return Vec2::ZERO;
}

void GameScene::refreshAllCards() {
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto& mainArea = _gameModel.getMainAreaCards();
    size_t numCols = mainArea.size();
    float cardScale = getCardScale();

    // --- Must match setupView parameters ---
    float sideMargin = visibleSize.width * 0.05f; 
    float availableWidth = visibleSize.width - (sideMargin * 2.0f);
    float gapX = availableWidth / static_cast<float>(numCols); 
    float startX = sideMargin + (gapX / 2.0f);
    float gapY = (190.0f * cardScale) * 0.45f;
    float startY = visibleSize.height * 0.75f;

    float handX = visibleSize.width / 2.0f;
    float handY = visibleSize.height * 0.15f;

    // Update Main Pile
    for (size_t i = 0; i < mainArea.size(); ++i) {
        for (size_t j = 0; j < mainArea[i].size(); ++j) {
            auto model = mainArea[i][j];
            if (_modelToSprite.count(model)) {
                auto sprite = _modelToSprite[model];
                sprite->stopAllActions();
                sprite->setScale(cardScale);
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
            sprite->setScale(cardScale);
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
            _modelToSprite[model]->setScale(cardScale);
        }
    }
    
    _stockPileSprite->setVisible(true); // Always visible to show count or refresh
    if (stockCards.empty()) {
        _stockCountLabel->setString("REFRESH");
        _stockCountLabel->setSystemFontSize(24);
        _stockPileSprite->setOpacity(128); // Faded when empty
    } else {
        _stockCountLabel->setString(std::to_string(stockCards.size()));
        _stockCountLabel->setSystemFontSize(40);
        _stockPileSprite->setOpacity(255);
    }
    _stockPileSprite->setScale(cardScale);
    _stockPileSprite->setPosition(Vec2(handX - (visibleSize.width * 0.25f), handY));

    // Update Progress
    int totalRemaining = 0;
    for (const auto& col : mainArea) totalRemaining += col.size();
    _progressLabel->setString("REMAINING: " + std::to_string(totalRemaining));
}
