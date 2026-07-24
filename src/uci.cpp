#include "uci.h"
#include "book.h"
#include "move_parser.h"
#include "order_moves.h"
#include "position.h"
#include "search.h"
#include "tt.h"
#include "types.h"
#include "utils.h"
#include <iostream>
#include <sstream>
#include <string>

UCIOptions uci_options;

static void print_id() {
  std::cout << "id name Endgame\n";
  std::cout << "id author Endgame\n";
}

static void print_options() {
  std::cout << "option name UCI_Chess960 type check default false\n";
  std::cout << "option name Hash type spin default 100 min 1 max 4096\n";
  std::cout << "option name Clear Hash type button\n";
  std::cout << "option name Book File type string default \n";
}

static void handle_setoption(const std::string &line) {
  std::istringstream ss(line);
  std::string token, name, value;
  ss >> token; // "setoption"
  ss >> token; // "name"

  // Read multi-word option name until "value" or end
  name.clear();
  while (ss >> token) {
    if (token == "value")
      break;
    if (!name.empty())
      name += ' ';
    name += token;
  }
  // Read value (rest of the line)
  std::getline(ss, value);
  if (!value.empty() && value[0] == ' ')
    value.erase(0, 1);

  if (name == "UCI_Chess960") {
    uci_options.chess960 = (value == "true");
  } else if (name == "Hash") {
    int mb = std::stoi(value);
    if (mb < 1)
      mb = 1;
    if (mb > 4096)
      mb = 4096;
    uci_options.hash_size = mb;
    // TT is fixed at 100 MB for now — resizing would need re-init
  } else if (name == "Clear Hash") {
    tt.table.assign(tt.size, TTEntry{});
  } else if (name == "Book File") {
    uci_options.book_file = value;
    book_open(value);
  }
}

static void handle_position(const std::string &line, Position &pos) {
  std::string rest = line.substr(9);

  if (rest.rfind("startpos", 0) == 0) {
    pos.set_startpos();
    rest = rest.substr(8);
  } else if (rest.rfind("fen", 0) == 0) {
    rest = rest.substr(4);
    auto moves_pos = rest.find(" moves ");
    std::string fen_part =
        (moves_pos == std::string::npos) ? rest : rest.substr(0, moves_pos);
    set_fen(pos, fen_part);
    rest = (moves_pos == std::string::npos) ? "" : rest.substr(moves_pos + 7);
  }

  auto moves_pos = line.find(" moves ");
  if (moves_pos != std::string::npos) {
    std::string moves_str = line.substr(moves_pos + 7);
    std::istringstream ms(moves_str);
    std::string mstr;
    while (ms >> mstr) {
      Move m = parse_uci_move(pos, mstr);
      if (m != Move{}) {
        StateInfo st;
        pos.make_move(m, st);
      }
    }
  }
}

static void handle_go(const std::string &line, Position &pos) {
  // Probe opening book first
  Move book_move = book_probe(pos);
  if (book_move != Move{}) {
    std::cout << "bestmove " << Util::move_to_string(book_move) << "\n";
    return;
  }

  SearchLimits limits = parse_go(line);
  iterative_deepening(pos, limits);
}

void uci_loop() {
  Position pos;
  pos.set_startpos();

  std::cout.setf(std::ios::unitbuf);

  std::string line;
  while (std::getline(std::cin, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();

    if (line == "uci") {
      print_id();
      print_options();
      std::cout << "uciok\n";

    } else if (line == "isready") {
      std::cout << "readyok\n";

    } else if (line == "ucinewgame") {
      tt.table.assign(tt.size, TTEntry{});
      clear_search_tables();
      pos.set_startpos();

    } else if (line.rfind("setoption", 0) == 0) {
      handle_setoption(line);

    } else if (line.rfind("position", 0) == 0) {
      handle_position(line, pos);

    } else if (line.rfind("go", 0) == 0) {
      handle_go(line, pos);

    } else if (line == "stop") {
      search_stopped = true;

    } else if (line == "quit") {
      search_stopped = true;
      break;
    }
  }
}
