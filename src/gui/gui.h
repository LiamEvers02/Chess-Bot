#pragma once
#include <SFML/Graphics.hpp>
#include "engine/position.h"
#include "engine/movegen.h"
#include "engine/attacks.h"

class GUI {
public:
    GUI();
    void run();

private:
    sf::RenderWindow window;
    sf::Texture pieceTexture;
    sf::Sprite pieceSprites[COLOR_NB][PIECE_TYPE_NB];
    Position pos;
    Square selected;

    void drawBoard();
    void drawPieces();
    void drawHighlights();
    void handleClick(int pixelX, int pixelY);
    Square pixelToSquare(int pixelX, int pixelY);
};