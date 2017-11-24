#include "Player.hh"

/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME Sugus_Pere

struct PLAYER_NAME : public Player {
  /**
   * Factory: returns a new instance of this class.
   * Do not modify this function.
   */
  static Player* factory() { return new PLAYER_NAME; }

  /**
   * Types and attributes for your player can be defined here.
   */
  // int types
  typedef vector<int> intV;
  typedef vector<intV> intMat;
  typedef map<int, int> intMap;
  // bool types
  typedef vector<bool> boolV;
  typedef vector<boolV> boolMat;
  // Pos types
  typedef queue<Pos> PosQ;
  typedef vector<Pos> posV;
  typedef map<Pos, Pos> posMap;

  // Position + distance
  struct dPos {
    Pos p;
    int d;
    dPos(Pos pos, int dist) : p(pos), d(dist) {}
    bool operator>(const dPos& other) const { return d > other.d; }
  };
  typedef priority_queue<dPos, vector<dPos>, greater<dPos>>
      PosPQ;  // TODO Maybe change Container??

  // Search comparisons
  enum cmpSearch { CMP_CITY, CMP_ENEMY };

  // Valid ork states
  enum orkStates {
    ORK_DEFAULT,
    ANSIAROTA_C,
    GUARD,
    KILLER,
    FLEE,
    SUICIDE,
    ORK_STATES_SIZE
  };

  // Probs
  intV probInitial = {ANSIAROTA_C, ANSIAROTA_C, ANSIAROTA_C, ANSIAROTA_C,
                      ANSIAROTA_C, ANSIAROTA_C, KILLER,      KILLER,
                      KILLER,      KILLER};

  // Controller variables:
  intMap status;

  // Data functions

  // Returrns percentage of my orks (0-100)
  int pctOrcos() {
    int total = nb_players() * nb_orks();
    int myOrks = orks(me()).size();
    return (100 * myOrks) / total;
  }

  // Indica si voy ganando
  bool winning() {
    for (int pl = 0; pl < nb_players(); ++pl)
      if (pl != me() and total_score(me()) <= total_score(pl)) return false;
    return true;
  }

  // Devuelve las casillas vecinas a la que es posible moverse
  void veins(Pos pos, posV& res, Unit myUnit) {
    for (int d = 0; d != DIR_SIZE; ++d) {
      Dir dir = Dir(d);
      Pos npos = pos + dir;
      if (pos_ok(npos) and cell(npos).type != WATER) {
        if (cell(npos).unit_id != -1) {
          Unit un = unit(cell(npos).unit_id);
          if (un.player == -1)
            res.push_back(npos);
          else if (un.player != me() and un.health < myUnit.health)
            res.push_back(npos);

        } else
          res.push_back(npos);
      }
    }
  }

  // Search Comparator
  bool cmp_search(cmpSearch ct, Cell& c, Unit& u) {
    if (ct == CMP_CITY)
      return (c.type == CITY and city_owner(c.city_id) != me()) or
             (c.type == PATH and path_owner(c.path_id) != me());
    else if (ct == CMP_ENEMY) {
      if (c.unit_id == -1) return false;
      int pid = unit(c.unit_id).player;
      return pid != -1 and
             unit(c.unit_id).health <
                 u.health;  // No se comprueba me() ya que lo hago en veins()
    }

    else
      return false;
  }

  // Basic BFS, returns direction to move to reach first City
  Dir bfs(Pos pos, cmpSearch ct, Unit u) {
    PosQ q;
    boolMat visited(rows(), boolV(cols(), false));
    posMap parents;
    visited[pos.i][pos.j] = true;
    q.push(pos);

    Pos p;
    bool found = false;
    while (!q.empty() and not found) {
      p = q.front();
      Cell c = cell(p);
      if (cmp_search(ct, c, u))
        found = true;
      else {
        q.pop();
        posV neig;
        veins(p, neig, u);
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

  // Dijkstra's algorithm for pretty orks
  Dir dijkstra(Pos pos, cmpSearch ct, Unit u) {
    PosPQ pq;
    intMat prices(rows(), intV(cols(), -1));
    posMap parents;
    prices[pos.i][pos.j] = 0;
    pq.push(dPos(pos, 0));

    dPos dp =
        pq.top();  // Initialized to valid value, will be overwritten later
    bool found = false;

    while (not found and not pq.empty()) {
      dp = pq.top();
      Cell c = cell(dp.p);
      if (cmp_search(ct, c, u))
        found = true;
      else {
        pq.pop();

        posV neig;
        veins(dp.p, neig, u);
        for (Pos n : neig) {
          int newd = dp.d + 1 + cost(cell(n.i, n.j).type);
          if (prices[n.i][n.j] == -1 or newd < prices[n.i][n.j]) {
            pq.push(dPos(n, newd));
            prices[n.i][n.j] = newd;
            parents[n] = dp.p;
          }
        }
      }
    }
    Pos p = dp.p;

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

  Dir behavior_killer(Unit u) {
    // TODO suicide if low health
    return dijkstra(u.pos, CMP_ENEMY, u);
  }
  /**
   * Moves the player, also state machine selection
   */
  void move(int id) {
    Unit u = unit(id);
    int s = status[id];
    Dir d;
    if (s == ORK_DEFAULT) {
      // Random initial status
      s = status[id] = probInitial[random(0, 9)];
    }
    if (s == ANSIAROTA_C) d = dijkstra(u.pos, CMP_CITY, u);

    if (s == KILLER) d = behavior_killer(u);
    execute(Command(id, d));
  }

  /**
   * Play method, invoked once per each round.
   */
  virtual void play() {
    intV m_orcos = orks(me());
    if (round() == 0) {
      // First round code
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
