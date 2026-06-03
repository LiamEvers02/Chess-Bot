#include "gui.h"
#include "search/search.h"
#include <SFML/Graphics.hpp>
#include <random>
#include <ctime>
#include <fstream>
#include <algorithm>

static const int TILE      = 80;
static const int BOARD_W   = TILE * 8;   // 640
static const int SIDEBAR_W = 220;
static const int WINDOW_W  = BOARD_W + SIDEBAR_W;
static const int WINDOW_H  = BOARD_W;

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

GUI::GUI() : window(sf::VideoMode(WINDOW_W, WINDOW_H), "Chess-Bot"), selected(NO_SQUARE), status(GameStatus::Playing) {
    init_attacks();
    Zobrist::init();
    pieceFont.loadFromFile("C:/Windows/Fonts/seguisym.ttf");
    uiFont.loadFromFile("C:/Windows/Fonts/arial.ttf");
    resetGame();
}

void GUI::resetGame() {
    set(pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    selected = NO_SQUARE;
    status = GameStatus::Playing;
    moveHistory.clear();
    capturedByWhite.clear();
    capturedByBlack.clear();
    std::mt19937 rng(static_cast<unsigned>(std::time(nullptr)));
    humanColor = (rng() % 2 == 0) ? WHITE : BLACK;
}

std::string GUI::moveToString(Move m) {
    static const char files[] = "abcdefgh";
    static const char pieceChar[] = " PNBRQK";

    Square from = from_sq(m);
    Square to = to_sq(m);
    MoveType mt = type_of(m);
    Piece p = pos.piece_on(from);
    PieceType pt = type_of(p);
    Color us = color_of(p);

    if (mt == CASTLING)
        return file_of(to) > file_of(from) ? "O-O" : "O-O-O";

    bool isCapture = (pos.piece_on(to) != NO_PIECE) || (mt == EN_PASSANT);
    std::string s;

    if (pt == PAWN) {
        if (isCapture) { s += files[file_of(from)]; s += 'x'; }
        s += files[file_of(to)];
        s += char('1' + rank_of(to));
        if (mt == PROMOTION) {
            static const char promo[] = {'n','b','r','q'};
            s += '=';
            s += char(std::toupper(promo[promotion_type(m) - KNIGHT]));
        }
    } else {
        s += pieceChar[pt];

        // Disambiguation: find other pieces of the same type that can reach 'to'
        auto legalMoves = generate_legal_moves(pos);
        bool sameFile = false, sameRank = false, ambig = false;
        for (Move o : legalMoves) {
            Square of = from_sq(o);
            if (of == from || to_sq(o) != to) continue;
            Piece op = pos.piece_on(of);
            if (type_of(op) != pt || color_of(op) != us) continue;
            ambig = true;
            if (file_of(of) == file_of(from)) sameFile = true;
            if (rank_of(of) == rank_of(from)) sameRank = true;
        }
        if (ambig) {
            if      (!sameFile) s += files[file_of(from)];
            else if (!sameRank) s += char('1' + rank_of(from));
            else { s += files[file_of(from)]; s += char('1' + rank_of(from)); }
        }

        if (isCapture) s += 'x';
        s += files[file_of(to)];
        s += char('1' + rank_of(to));
    }

    return s;
}

void GUI::applyMove(Move m) {
    // Record capture before the move changes the board
    Piece captured = NO_PIECE;
    if (type_of(m) == EN_PASSANT)
        captured = make_piece(~pos.side_to_move(), PAWN);
    else
        captured = pos.piece_on(to_sq(m));

    if (captured != NO_PIECE) {
        if (pos.side_to_move() == WHITE)
            capturedByWhite.push_back(captured);
        else
            capturedByBlack.push_back(captured);
    }

    std::string san = moveToString(m);
    do_move(pos, m);

    // Append check (+) or checkmate (#) suffix
    auto responses = generate_legal_moves(pos);
    Bitboard kings = pos.pieces(pos.side_to_move(), KING);
    if (kings) {
        Square ks = lsb(kings);
        if (is_square_attacked(pos, ks, ~pos.side_to_move()))
            san += responses.empty() ? '#' : '+';
    }

    moveHistory.push_back(san);
    selected = NO_SQUARE;
    checkGameStatus();
}

void GUI::checkGameStatus() {
    auto moves = generate_legal_moves(pos);
    if (!moves.empty()) { status = GameStatus::Playing; return; }
    Bitboard kings = pos.pieces(pos.side_to_move(), KING);
    Square ks = kings ? lsb(kings) : NO_SQUARE;
    if (ks != NO_SQUARE && is_square_attacked(pos, ks, ~pos.side_to_move()))
        status = GameStatus::Checkmate;
    else
        status = GameStatus::Stalemate;
}

void GUI::saveGame() {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "game_%Y%m%d_%H%M%S.pgn", std::localtime(&t));
    std::ofstream file(buf);
    char dateBuf[16];
    std::strftime(dateBuf, sizeof(dateBuf), "%Y.%m.%d", std::localtime(&t));
    file << "[Event \"Chess-Bot Game\"]\n[Date \"" << dateBuf << "\"]\n\n";
    for (int i = 0; i < (int)moveHistory.size(); i++) {
        if (i % 2 == 0) file << (i / 2 + 1) << ". ";
        file << moveHistory[i] << " ";
    }
    file << "\n";
}

void GUI::engineMove() {
    auto moves = generate_legal_moves(pos);
    if (moves.empty()) return;
    applyMove(best_move(pos, 3));
}

void GUI::run() {
    while (window.isOpen()) {
        if (status == GameStatus::Playing && pos.side_to_move() != humanColor){
            sf::sleep(sf::milliseconds(200));  // Brief pause before engine moves
            engineMove();
        }

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            else if (event.type == sf::Event::MouseButtonPressed)
                handleClick(event.mouseButton.x, event.mouseButton.y);
        }
        window.clear(sf::Color(30, 30, 30));
        drawBoard();
        drawHighlights();
        drawPieces();
        drawSidebar();
        if (status != GameStatus::Playing)
            drawEndScreen();
        window.display();
    }
}

int GUI::screenRank(int rank) const {
    return (humanColor == WHITE) ? (7 - rank) : rank;
}

void GUI::drawBoard() {
    sf::RectangleShape tile(sf::Vector2f(TILE, TILE));
    for (int r = 0; r < 8; r++) {
        for (int f = 0; f < 8; f++) {
            tile.setPosition(f * TILE, screenRank(r) * TILE);
            tile.setFillColor(((r + f) % 2 == 0) ? LIGHT : DARK);
            window.draw(tile);
        }
    }

    // Rank numbers (1-8) on left edge
    static const char rankLabels[] = "12345678";
    static const char fileLabels[] = "abcdefgh";
    for (int i = 0; i < 8; i++) {
        sf::Text t; t.setFont(uiFont); t.setCharacterSize(12);
        bool lightSquare = (i % 2 == 0);
        t.setFillColor(lightSquare ? DARK : LIGHT);

        // Rank label
        t.setString(std::string(1, rankLabels[i]));
        t.setPosition(2.f, screenRank(i) * TILE + 2.f);
        window.draw(t);

        // File label
        t.setString(std::string(1, fileLabels[humanColor == WHITE ? i : 7 - i]));
        t.setPosition(i * TILE + TILE - 12.f, BOARD_W - 16.f);
        window.draw(t);
    }

    // Player side indicator at the bottom
    sf::Text side; side.setFont(uiFont); side.setCharacterSize(15);
    side.setFillColor(sf::Color(220, 220, 220));
    side.setString(std::string("You: ") + (humanColor == WHITE ? "White" : "Black"));
    sf::FloatRect sb = side.getLocalBounds();
    side.setPosition((BOARD_W - sb.width) / 2.f - sb.left, BOARD_W - 20.f);
    window.draw(side);
}

void GUI::drawHighlights() {
    sf::RectangleShape tile(sf::Vector2f(TILE, TILE));

    // Red highlight if the side to move's king is in check
    Bitboard kings = pos.pieces(pos.side_to_move(), KING);
    if (kings) {
        Square ks = lsb(kings);
        if (is_square_attacked(pos, ks, ~pos.side_to_move())) {
            tile.setFillColor(sf::Color(220, 50, 50, 200));
            tile.setPosition(file_of(ks) * TILE, screenRank(rank_of(ks)) * TILE);
            window.draw(tile);
        }
    }

    if (selected == NO_SQUARE) return;

    // Highlight selected square
    tile.setFillColor(SEL);
    tile.setPosition(file_of(selected) * TILE, screenRank(rank_of(selected)) * TILE);
    window.draw(tile);

    // Highlight legal moves
    tile.setFillColor(HINT);
    for (Move m : generate_legal_moves(pos)) {
        if (from_sq(m) != selected) continue;
        Square to = to_sq(m);
        tile.setPosition(file_of(to) * TILE, screenRank(rank_of(to)) * TILE);
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
        text.setFont(pieceFont);
        text.setString(sf::String(PIECE_GLYPH[c][pt]));
        text.setCharacterSize(60);
        text.setFillColor(c == WHITE ? sf::Color::White : sf::Color(30, 30, 30));
        text.setOutlineColor(c == WHITE ? sf::Color(80, 80, 80) : sf::Color::White);
        text.setOutlineThickness(1.5f);

        // Centre glyph within tile
        sf::FloatRect bounds = text.getLocalBounds();
        float px = file_of(s) * TILE + (TILE - bounds.width)  / 2.f - bounds.left;
        float py = screenRank(rank_of(s)) * TILE + (TILE - bounds.height) / 2.f - bounds.top;
        text.setPosition(px, py);
        window.draw(text);
    }
}

void GUI::drawSidebar() {
    const float x0 = BOARD_W + 10.f;
    float y = 10.f;

    auto label = [&](const std::string& str, unsigned size, sf::Color col) {
        sf::Text t; t.setFont(uiFont); t.setCharacterSize(size);
        t.setFillColor(col); t.setString(str); return t;
    };

    // Captured by white (these are black pieces)
    { auto l = label("White captured:", 14, sf::Color(180,180,180)); l.setPosition(x0,y); window.draw(l); y+=20.f;
      sf::Text gt; gt.setFont(pieceFont); gt.setCharacterSize(26); gt.setFillColor(sf::Color(50,50,50));
      float cx=x0;
      for (Piece p : capturedByWhite) {
          gt.setString(sf::String(PIECE_GLYPH[color_of(p)][type_of(p)]));
          gt.setPosition(cx,y); window.draw(gt); cx+=28.f;
          if (cx > BOARD_W+SIDEBAR_W-10) { cx=x0; y+=30.f; }
      } y+=32.f; }

    // Captured by black (these are white pieces)
    { auto l = label("Black captured:", 14, sf::Color(180,180,180)); l.setPosition(x0,y); window.draw(l); y+=20.f;
      sf::Text gt; gt.setFont(pieceFont); gt.setCharacterSize(26); gt.setFillColor(sf::Color(220,220,220));
      float cx=x0;
      for (Piece p : capturedByBlack) {
          gt.setString(sf::String(PIECE_GLYPH[color_of(p)][type_of(p)]));
          gt.setPosition(cx,y); window.draw(gt); cx+=28.f;
          if (cx > BOARD_W+SIDEBAR_W-10) { cx=x0; y+=30.f; }
      } y+=32.f; }

    // Separator
    { sf::RectangleShape sep(sf::Vector2f(SIDEBAR_W-20.f, 1.f));
      sep.setPosition(x0, y); sep.setFillColor(sf::Color(80,80,80)); window.draw(sep); y+=10.f; }

    // Move history
    { auto l = label("Moves", 14, sf::Color(180,180,180)); l.setPosition(x0,y); window.draw(l); y+=20.f;
      sf::Text mt; mt.setFont(uiFont); mt.setCharacterSize(13); mt.setFillColor(sf::Color(220,220,220));
      int total = (int)moveHistory.size();
      int pairs = (total+1)/2;
      int maxLines = (int)((WINDOW_H - y - 10) / 16);
      int startPair = std::max(0, pairs - maxLines);
      for (int i = startPair; i < pairs && y < WINDOW_H-10; i++) {
          std::string line = std::to_string(i+1) + ". " + moveHistory[i*2];
          if (i*2+1 < total) line += "  " + moveHistory[i*2+1];
          mt.setString(line); mt.setPosition(x0, y); window.draw(mt); y+=16.f;
      } }
}

void GUI::drawEndScreen() {
    // Grey box, no frills
    sf::RectangleShape box(sf::Vector2f(300.f, 160.f));
    box.setFillColor(sf::Color(200, 200, 200));
    box.setPosition(100.f, 200.f);
    window.draw(box);

    sf::Text result; result.setFont(uiFont); result.setCharacterSize(20);
    result.setFillColor(sf::Color::Black);
    if (status == GameStatus::Checkmate) {
        Color winner = ~pos.side_to_move();
        result.setString(winner == WHITE ? "White wins (checkmate)" : "Black wins (checkmate)");
    } else {
        result.setString("Stalemate");
    }
    result.setPosition(110.f, 210.f);
    window.draw(result);

    // Save button: plain grey rectangle with black text
    sf::RectangleShape saveBtn(sf::Vector2f(100.f, 30.f));
    saveBtn.setFillColor(sf::Color(180, 180, 180));
    saveBtn.setPosition(110.f, 270.f);
    window.draw(saveBtn);
    sf::Text saveTxt; saveTxt.setFont(uiFont); saveTxt.setCharacterSize(16);
    saveTxt.setFillColor(sf::Color::Black); saveTxt.setString("Save");
    saveTxt.setPosition(115.f, 274.f);
    window.draw(saveTxt);

    // Rematch button: same
    sf::RectangleShape rematchBtn(sf::Vector2f(100.f, 30.f));
    rematchBtn.setFillColor(sf::Color(180, 180, 180));
    rematchBtn.setPosition(220.f, 270.f);
    window.draw(rematchBtn);
    sf::Text rematchTxt; rematchTxt.setFont(uiFont); rematchTxt.setCharacterSize(16);
    rematchTxt.setFillColor(sf::Color::Black); rematchTxt.setString("Rematch");
    rematchTxt.setPosition(225.f, 274.f);
    window.draw(rematchTxt);
}

void GUI::handleClick(int pixelX, int pixelY) {
    // End screen buttons
    if (status != GameStatus::Playing) {
        if (pixelY >= 270 && pixelY <= 300) {
            if (pixelX >= 110 && pixelX <= 210) saveGame();
            if (pixelX >= 220 && pixelX <= 320) resetGame();
        }
        return;
    }

    if (pos.side_to_move() != humanColor) return;

    Square clicked = pixelToSquare(pixelX, pixelY);
    if (clicked == NO_SQUARE) return;

    if (selected != NO_SQUARE) {
        bool moved = false;
        for (Move m : generate_legal_moves(pos)) {
            if (from_sq(m) == selected && to_sq(m) == clicked) {
                applyMove(m); moved = true; break;
            }
        }
        if (!moved) {
            selected = NO_SQUARE;
            Piece p = pos.piece_on(clicked);
            if (p != NO_PIECE && color_of(p) == pos.side_to_move())
                selected = clicked;
        }
    } else {
        Piece p = pos.piece_on(clicked);
        if (p != NO_PIECE && color_of(p) == pos.side_to_move())
            selected = clicked;
    }
}

Square GUI::pixelToSquare(int pixelX, int pixelY) const {
    if (pixelX >= BOARD_W) return NO_SQUARE;
    int file = pixelX / TILE;
    int screenR = pixelY / TILE;
    if (file < 0 || file > 7 || screenR < 0 || screenR > 7) return NO_SQUARE;
    int rank = (humanColor == WHITE) ? (7 - screenR) : screenR;
    return make_square(file, rank);
}