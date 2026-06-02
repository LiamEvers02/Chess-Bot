#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "engine/position.h"
#include "engine/movegen.h"
#include "engine/attacks.h"

enum class GameStatus { Playing, Checkmate, Stalemate };

class GUI {
public:
    GUI();
    void run();

private:
    sf::RenderWindow window;
    sf::Font pieceFont;
    sf::Font uiFont;
    Position pos;
    Square selected;
    Color humanColor;
    GameStatus status;

    std::vector<std::string> moveHistory;
    std::vector<Piece> capturedByWhite;  // black pieces white has taken
    std::vector<Piece> capturedByBlack;  // white pieces black has taken

    void engineMove();
    void applyMove(Move m);
    void resetGame();
    void saveGame();
    void checkGameStatus();
    std::string moveToString(Move m);  // SAN — called before do_move

    int screenRank(int rank) const;  // flips rank when human plays black

    void drawBoard();
    void drawPieces();
    void drawHighlights();
    void drawSidebar();
    void drawEndScreen();
    void handleClick(int pixelX, int pixelY);
    Square pixelToSquare(int pixelX, int pixelY) const;
};