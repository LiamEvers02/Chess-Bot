#include "gui.h"
#include <SFML/Graphics.hpp>

static const int TILE = 80;
static const int W = TILE * 8;

static sf::Color LIGHT = sf::Color(240, 217, 181);
static sf::Color DARK  = sf::Color(181, 136,  99);
static sf::Color SEL   = sf::Color(100, 200, 100, 150);
static sf::Color HINT  = sf::Color(100, 200, 100,  80);

// Unicode chess symbols indexed by [Color][PieceType]
// Order matches PieceType enum: 0=NO_PIECE_TYPE,1=P,2=N,3=B,4=R,5=Q,6=K
static const wchar_t* PIECE_GLYPH[COLOR_NB][PIECE_TYPE_NB] = {
    { L"", L"\u2659", L"\u2658", L"\u2657", L"\u2656", L"\u2655", L"\u2654" }, // White
    { L"", L"\u265F", L"\u265E", L"\u265D", L"\u265C", L"\u265B", L"\u265A" }, // Black
};

GUI::GUI() : window(sf::VideoMode(W, W), "Chess-Bot"), selected(NO_SQUARE) {
    init_attacks();
    set(pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    font.loadFromFile("C:/Windows/Fonts/seguisym.ttf");
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
    for (Square s = A1; s < SQUARE_NB; s = Square(s + 1)) {
        Piece p = pos.piece_on(s);
        if (p == NO_PIECE) continue;
        PieceType pt = type_of(p);
        Color c = color_of(p);

        sf::Text text;
        text.setFont(font);
        text.setString(sf::String(PIECE_GLYPH[c][pt]));
        text.setCharacterSize(60);
        text.setFillColor(c == WHITE ? sf::Color::White : sf::Color(30, 30, 30));
        text.setOutlineColor(c == WHITE ? sf::Color(80, 80, 80) : sf::Color::White);
        text.setOutlineThickness(1.5f);

        // Centre glyph within tile
        sf::FloatRect bounds = text.getLocalBounds();
        float px = file_of(s) * TILE + (TILE - bounds.width)  / 2.f - bounds.left;
        float py = (7 - rank_of(s)) * TILE + (TILE - bounds.height) / 2.f - bounds.top;
        text.setPosition(px, py);
        window.draw(text);
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