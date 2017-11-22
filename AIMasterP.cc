#include "Player.hh"

/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME MasterP

struct PLAYER_NAME : public Player {
  /**
   * Factory: returns a new instance of this class.
   * Do not modify this function.
   */
  static Player *factory() { return new PLAYER_NAME; }

  /**
   * Types and attributes for your player can be defined here.
   */

  typedef vector<int> intV;
  typedef map<int, int> intMap;

  typedef vector<intV> intMat;
  typedef vector<bool> boolV;
  typedef vector<boolV> boolMat;
  typedef queue<Pos> PosQ;
  typedef vector<Pos> posV;
  /* typedef struct vPos {
    Pos m, p;
    vPos(Pos me) : m(me), p(Pos(-1, -1)) {}
    vPos(Pos me, Pos par) : m(me), p(par) {}
  }; */
  typedef map<Pos, Pos> posMap;

  enum orkStates { SEARCH_C, SEARCH_P, KEEP, KILL, ORK_STATES_SIZE };

  // Controller variables:
  bool first = true;  // Run initial setup
  intMap status;

  // Devuelve las casillas vecinas a la que es posible moverse
  void veins(Pos pos, posV &res) {
    for (int d = 0; d != DIR_SIZE; ++d) {
      Dir dir = Dir(d);
      Pos npos = pos + dir;
      if (pos_ok(npos) and cell(npos).type != WATER) {
        res.push_back(npos);
      }
    }
  }

  bool cmp_search(CellType ct, Cell c) {
    if (ct == CITY)
      return c.type == CITY and city_owner(c.city_id) != me();
    else if (ct == PATH)
      return c.type == PATH and path_owner(c.path_id) != me();

    else
      return false;
  }

  // Basic BFS, returns direction to move to reach first City
  Dir bfs(Pos pos, CellType ct) {
    PosQ q;
    boolMat visited(rows(), boolV(cols(), false));
    posMap parents;
    visited[pos.i][pos.j] = true;
    q.push(pos);
    posV neig;

    Pos p;
    bool found = false;
    while (!q.empty() and not found) {
      p = q.front();
      Cell c = cell(p);
      if (cmp_search(ct, c))
        found = true;
      else {
        q.pop();
        neig.clear();
        veins(p, neig);
        for (Pos n : neig) {
          if (not visited[n.i][n.j]) {
            q.push(n);
            parents[n] = p;
            visited[n.i][n.j] = true;
          }
        }
      }
    }
    while (p != pos and parents[p] != pos) p = parents[p];
    if (p == pos) return Dir(NONE);

    // Calculate Initial direction
    for (int d = 0; d != DIR_SIZE; ++d) {
      Dir dir = Dir(d);
      Pos npos = pos + dir;
      if (npos == p) {
        return dir;
      }
    }

    _unreachable();
  }

  /**
   * Moves the player
   */
  void move(int id) {
    Unit u = unit(id);
    int s = status[id];
    Dir d;
    if (s == SEARCH_C) d = bfs(u.pos, CITY);
    if (s == SEARCH_P) d = bfs(u.pos, PATH);
    execute(Command(id, d));
  }

  /**
   * Play method, invoked once per each round.
   */
  virtual void play() {
    intV m_orcos = orks(me());
    if (first) {
      first = false;
      intV randP = random_permutation(m_orcos.size());
      int count = 0;
      for (int rd : randP) {
        if (count < m_orcos.size() / 2)
          status[m_orcos[rd]] = SEARCH_C;
        else
          status[m_orcos[rd]] = SEARCH_P;
        count++;
      }
    }

    for (int id : m_orcos) {
      move(id);
    }
  }
};

/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);
