#include "gui.h"
#include <SFML/Graphics.hpp>

static const int TILE = 80; // Size of each square in pixels
static const int W = TILE * 8;

static sf::Color LIGHT = sf::Color(240, 217, 181);
static sf::Color DARK = sf::Color(181, 136, 99);
static sf::Color SEL = sf::Color(100, 200, 100, 150);
static sf::Color HINT = sf::Color(100, 200, 100, 80);

GUI::GUI() : window(sf::VideoMode(W, W), "Chess-Bot"), selected(NO_SQUARE) {
    init_attacks();
    set(pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    pieceTexture.loadFromFile("assets/pieces.png");

    int sheetOrder[] = {KING, QUEEN, ROOK, BISHOP, KNIGHT, PAWN};
    int tileW = pieceTexture.getSize().x / 6;
    int tileH = pieceTexture.getSize().y / 2;
    for (int col = 0; col < 6; col++){
        for (int row = 0; row < 2; row++){
            pieceSprites[row][sheetOrder[col]].setTexture(pieceTexture);
            pieceSprites[row][sheetOrder[col]].setTextureRect(
                sf::IntRect(col * tileW, row * tileH, tileW, tileH)
            );
            float scale = float(TILE) / tileW;
            pieceSprites[row][sheetOrder[col]].setScale(scale, scale);
        }
    }
}

void GUI::run() {
    while (window.isOpen()){
        sf::Event event;
        while (window.pollEvent(event)){
            if (event.type == sf::Event::Closed){
                window.close();
            }
            else if (event.type == sf::Event::MouseButtonPressed){
                handleClick(event.mouseButton.x, event.mouseButton.y);
            }
        }
        window.clear();
        drawBoard();
        drawHighlights();
        drawPieces();
        window.display();
    }
}

void GUI::drawBoard() {
    sf::RectangleShape tile(sf::Vector2f(TILE, TILE));
    for (int r = 0; r < 8; r++){
        for (int f = 0; f < 8; f++){
            tile.setPosition(f * TILE, (7 - r) * TILE);
            tile.setFillColor(((r + f) % 2 == 0) ? LIGHT : DARK);
            window.draw(tile);
        }
    }
}

void GUI::drawHighlights() {
    if (selected == NO_SQUARE) return;
    sf::RectangleShape tile(sf::Vector2f(TILE, TILE));

    // Highlight selected square
    tile.setFillColor(SEL);
    tile.setPosition((file_of(selected)) * TILE, (7 - rank_of(selected)) * TILE);
    window.draw(tile);

    // Highlight legal moves
    tile.setFillColor(HINT);
    for (Move m : generate_legal_moves(pos)){
        if (from_sq(m) != selected) continue;
        Square to = to_sq(m);
        tile.setPosition(file_of(to) * TILE, (7 - rank_of(to)) * TILE);
        window.draw(tile);
    }
}

void GUI::drawPieces() {
    for (Square s = A1; s < SQUARE_NB; s = Square(s + 1)){
        Piece p = pos.piece_on(s);
        if (p == NO_PIECE) continue;
        PieceType pt = type_of(p);
        Color c = color_of(p);
        pieceSprites[c][pt].setPosition(file_of(s) * TILE, (7 - rank_of(s)) * TILE);
        window.draw(pieceSprites[c][pt]);
    }
}

void GUI::handleClick(int pixelX, int pixelY) {
    Square clicked = pixelToSquare(pixelX, pixelY);
    if (clicked == NO_SQUARE) return;

    if (selected != NO_SQUARE) {
        // A piece is already selected — try to move it
        bool moved = false;
        for (Move m : generate_legal_moves(pos)) {
            if (from_sq(m) == selected && to_sq(m) == clicked) {
                do_move(pos, m);
                moved = true;
                break;
            }
        }
        selected = NO_SQUARE;
        // If the move failed and user clicked a friendly piece, select that instead
        if (!moved) {
            Piece p = pos.piece_on(clicked);
            if (p != NO_PIECE && color_of(p) == pos.side_to_move())
                selected = clicked;
        }
    } else {
        // Nothing selected — select a friendly piece
        Piece p = pos.piece_on(clicked);
        if (p != NO_PIECE && color_of(p) == pos.side_to_move())
            selected = clicked;
    }
}

Square GUI::pixelToSquare(int pixelX, int pixelY){
    int file = pixelX / TILE;
    int rank = 7 - (pixelY / TILE);
    if (file < 0 || file > 7 || rank < 0 || rank > 7) return NO_SQUARE;
    return make_square(file, rank);
}