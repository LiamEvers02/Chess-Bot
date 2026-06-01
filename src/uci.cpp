#include "uci.h"
#include "position.h"
#include "movegen.h"
#include "attacks.h"
#include <iostream>
#include <sstream>
#include <string>

static Position pos;

static void parse_position(const std::string& line){
    std::istringstream ss(line);
    std::string token;
    ss >> token; // consume "position"
    ss >> token; // "startpos" or "fen"
    if (token == "startpos"){
        set(pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        ss >> token; // consume "moves" if present
    }
    else if (token == "fen"){
        std::string fen;
        while (ss >> token && token != "moves"){
            fen += token + " ";
        }
        set(pos, fen);
        // token is now "moves" or ss is exhausted
    }
    // Apply move list — token may already be the first move (fen case) or next token (startpos case)
    while (ss >> token){
        Square from = make_square(token[0] - 'a', token[1] - '1');
        Square to = make_square(token[2] - 'a', token[3] - '1');
        for (Move m : generate_legal_moves(pos)){
            if (from_sq(m) == from && to_sq(m) == to){
                do_move(pos, m);
                break;
            }
        }
    }

}

static void parse_go() {
    // For now: return the first legal move
    auto moves = generate_legal_moves(pos);
    if (moves.empty()) { std::cout << "bestmove 0000\n"; return;}
    Move m = moves[0];
    std::string mv = "";
    mv += char('a' + file_of(from_sq(m)));
    mv += char('1' + rank_of(from_sq(m)));
    mv += char('a' + file_of(to_sq(m)));
    mv += char('1' + rank_of(to_sq(m)));
    if (type_of(m) == PROMOTION){
        const char promo[] = {'n', 'b', 'r', 'q'};
        mv += promo[promotion_type(m) - KNIGHT];
    }
    std::cout << "bestmove " << mv << "\n";
}

void uci_loop(){
    init_attacks();
    std::cout << "id name Chess-Bot\n";
    std::cout << "id author Liam";
    std::cout << "uciok\n";

    std::string line;
    while (std::getline(std::cin, line)){
        if (line == "uci") {
            std::cout << "id name Chess-Bot\n";
            std::cout << "id author Liam";
            std::cout << "uciok\n";
        }
        else if (line == "isready"){
            std::cout << "readyok\n";
        }
        else if (line == "ucinewgame"){
            set(pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        }
        else if (line.rfind("position", 0) == 0){
            parse_position(line);
        }
        else if (line.rfind("go", 0) == 0){
            parse_go();
        }
        else if (line == "quit"){
            break;
        }
        std::cout << std::flush;
    }
}