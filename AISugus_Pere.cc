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
  typedef vector<posV> posMat;
  typedef map<Pos, Pos> posMap;
  // misc types
  typedef map<int, Dir> dirMap;

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

  // Game states
  enum gameStates { DEFAULT, ANSIAROTA, KILL, GS_SIZE };
  bool LPMODE = false;  // Low-power mode
  int gameState = DEFAULT;
  // Probs
  const intV probDefault = {ANSIAROTA_C, ANSIAROTA_C, ANSIAROTA_C, ANSIAROTA_C,
                            ANSIAROTA_C, ANSIAROTA_C, ANSIAROTA_C, KILLER,
                            KILLER,      KILLER};
  const intV probAnsiarota = {
      ANSIAROTA_C, ANSIAROTA_C, ANSIAROTA_C, ANSIAROTA_C, ANSIAROTA_C,
      ANSIAROTA_C, ANSIAROTA_C, ANSIAROTA_C, ANSIAROTA_C, KILLER};
  const intV probKiller = {ANSIAROTA_C, KILLER, KILLER, KILLER, KILLER,
                           KILLER,      KILLER, KILLER, KILLER, KILLER};
  const intV* actualProbs = &probDefault;

  // Probabilities getter
  const intV* getProbs(int state) {
    if (state == DEFAULT) return &probDefault;
    if (state == ANSIAROTA) return &probAnsiarota;
    if (state == KILL) return &probKiller;
    _unreachable();
  }

  // Controller variables:
  intMap orkStatus;
  dirMap nextPos;
  intMat enemys;
  boolMat free;
  size_t nOrks;
  // Algo efficiency test:
  // int total_pass = 0;

  // Prints a int matrix
  /*void printMat(const intMat& m) {
    cerr << "Matrix:" << endl;
    for (intV row : m) {
      for (int el : row) {  // Dat C++11
        cerr << el << " ";
      }
      cerr << endl;
    }
    cerr << ":xirtaM" << endl;
  }*/

  // Data functions

  // Sets enemy position, if possible
  void setEnemy(int i, int j, int health) {
    if (i < 0 or j < 0) return;
    if (i > rows() - 1 or j > cols() - 1) return;
    if (cell(Pos(i, j)).type == WATER) return;
    int coste = cost(cell(Pos(i, j)).type);
    enemys[i][j] = max(enemys[i][j], health + coste);
  }

  // Set enemy pos for n=2
  void getEnemys() {
    enemys = intMat(rows(), intV(cols(), -1));
    Unit ork;
    // cerr << "OOOOORKKKS" << endl;
    for (int o = 0; o < nb_units(); ++o) {
      ork = unit(o);
      int& i = ork.pos.i;
      int& j = ork.pos.j;
      if (ork.player != me()) {
        // cerr << i << " " << j << endl;
        // Position
        setEnemy(i, j, ork.health);
        // 1 mov ahead
        setEnemy(i - 1, j, ork.health);
        setEnemy(i, j - 1, ork.health);
        setEnemy(i + 1, j, ork.health);
        setEnemy(i, j + 1, ork.health);
        // 2 ahead
        setEnemy(i - 2, j, ork.health);
        setEnemy(i, j - 2, ork.health);
        setEnemy(i + 2, j, ork.health);
        setEnemy(i, j + 2, ork.health);
        setEnemy(i + 1, j + 1, ork.health);
        setEnemy(i + 1, j - 1, ork.health);
        setEnemy(i - 1, j + 1, ork.health);
        setEnemy(i - 1, j - 1, ork.health);
      }
    }
  }

  // Returns percentage of my orks (0-100)
  int pctOrcos() {
    int total = nb_players() * nb_orks();

    int myOrks = orks(me()).size();
    return (100 * myOrks) / total;
  }

  // Indica si voy ganando
  bool winning() {
    int max = 0;
    // cerr << "state: **************************" << endl;
    for (int pl = 0; pl < nb_players(); ++pl) {
      int sc = total_score(pl);
      if (sc > max) max = total_score(pl);
      // cerr << "state: score of " << pl << "=" << sc << endl;
    }
    // cerr << "state: " << me() << endl;
    return max == total_score(me());
  }

  // Devuelve las casillas vecinas a la que es posible moverse
  void veins(Pos pos, posV& res, Unit myUnit) {
    for (int d = 0; d != NONE; ++d) {
      Dir dir = Dir(d);
      Pos npos = pos + dir;
      if (pos_ok(npos) and cell(npos).type != WATER) {
        res.push_back(npos);
      }
    }
  }

  // Checks position for enemy
  bool hasBeefyEnemy(Pos p, Unit myUnit) {
    if (enemys[p.i][p.j] > myUnit.health)
      return true;
    else
      return false;
  }

  // Search Comparator
  bool cmp_search(cmpSearch ct, const Cell& c, const Unit& u) {
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

  /***************
   * SEARCH ALGS *
   ***************/

  // Dijkstra's algorithm for pretty orks
  pair<Dir, Dir> dijkstra(const Pos& pos, cmpSearch ct, const Unit& u) {
    intMat prices(rows(), intV(cols(), -1));
    posMat parents(rows(), posV(cols()));
    boolMat visited(rows(), boolV(cols(), false));
    PosPQ pq;
    pq.push(dPos(pos, 0));

    bool found = false;
    Pos p = pos;
    while (not found and not pq.empty()) {
      p = pq.top().p;
      pq.pop();
      if (not visited[p.i][p.j]) {
        Cell c = cell(p.i, p.j);
        if (p != pos and
            hasBeefyEnemy(p, u))  // Enemy check (pos ho fem despres)
          visited[p.i][p.j] = true;
        else if (cmp_search(ct, c, u))  // Found check
          found = true;
        else {  // Calculate next
          visited[p.i][p.j] = true;
          posV res;
          veins(p, res, u);
          for (Pos v : res) {
            int c = 1 + (500 / (u.health + 10)) *
                            cost(cell(v.i, v.j).type);  // Cell change cost
            if (free[v.i][v.j] and (prices[v.i][v.j] == -1 or
                                    prices[v.i][v.j] > prices[p.i][p.j] + c)) {
              prices[v.i][v.j] = prices[p.i][p.j] + c;
              parents[v.i][v.j] = p;
              pq.push(dPos(v, prices[v.i][v.j]));
            }
          }
        }
      }
    }
    // Not found
    if (p == pos) return make_pair(Dir(NONE), Dir(NONE));

    Pos lastp = p;
    pair<Dir, Dir> dp;
    // Calculate the first 2 positions
    while (parents[p.i][p.j] != pos) {
      lastp = p;
      p = parents[p.i][p.j];
    }
    // First direction
    for (int d = 0; d != DIR_SIZE; ++d) {
      if (pos + Dir(d) == p) {
        dp.first = Dir(d);
        /*Hecto*/ break;  // Aaaargh!!!!!! (TODO: pensar algo mejor)
      }
    }
    // Second dir
    for (int d = 0; d != DIR_SIZE; ++d) {
      if (p + Dir(d) == lastp) {
        dp.second = Dir(d);
        return dp;
      }
    }

    _unreachable();
  }

  // Killer behavior
  Dir behavior_killer(Unit u) {
    // TODO suicide if low health
    return dijkstra(u.pos, CMP_ENEMY, u).first;
  }

  // Capture behavior
  Dir behavior_ansiarota(const Unit& u, int id) {
    if (round() % 2 == 0 or not LPMODE) {
      pair<Dir, Dir> dv = dijkstra(u.pos, CMP_CITY, u);

      nextPos[id] = dv.second;
      return dv.first;
    } else
      return nextPos[id];
  }

  // Selects direction to move when blocked
  Dir microMax(Pos p, Unit my) {
    int minDist = INT_MAX;
    int uid = -1;
    for (int o = 0; o < nb_units(); ++o) {
      Unit u = unit(o);
      if (u.player != me() and u.health > my.health) {
        int dist = abs(p.i - u.pos.i) + abs(p.j - u.pos.j);
        if (dist < minDist) {
          minDist = dist;
          uid = u.id;
        }
      }
    }
    if (uid == -1) return Dir(NONE);
    Pos enp = unit(uid).pos;
    int coste = INT_MAX;
    Dir out = Dir(NONE);
    // Bottom
    if (enp.i <= p.i) {
      Cell c = cell(p + Dir(BOTTOM));
      if (c.type != WATER and cost(c.type) < coste) {
        coste = cost(c.type);
        out = Dir(BOTTOM);
      }
    }
    // Top
    if (enp.i >= p.i) {
      Cell c = cell(p + Dir(TOP));
      if (c.type != WATER and cost(c.type) < coste) {
        coste = cost(c.type);
        out = Dir(TOP);
      }
    }
    // Left
    if (enp.j >= p.j) {
      Cell c = cell(p + Dir(LEFT));
      if (c.type != WATER and cost(c.type) < coste) {
        coste = cost(c.type);
        out = Dir(LEFT);
      }
    }
    // Right
    if (enp.j <= p.j) {
      Cell c = cell(p + Dir(RIGHT));
      if (c.type != WATER and cost(c.type) < coste) {
        coste = cost(c.type);
        out = Dir(RIGHT);
      }
    }
    // cerr << me() << "_state: Moving from " << p.i << "," << p.j << " in dir "
    //    << out << endl;
    return out;
  }

  /**
   * Moves the player, also state machine selection
   */
  void move(int id) {
    Unit u = unit(id);
    int s = orkStatus[id];
    Dir d = Dir(NONE);
    if (s == ORK_DEFAULT) {
      // Random initial status
      s = orkStatus[id] = (*actualProbs)[random(0, 9)];
      // cerr << "state: new guy in town: " << id << endl;
    }
    if (s == ANSIAROTA_C) {
      d = behavior_ansiarota(u, id);
    }

    if (s == KILLER) d = behavior_killer(u);

    if (d == NONE) {
      d = microMax(u.pos, u);
    }

    Pos nxt = u.pos + d;
    // enemys[u.pos.i][u.pos.j] -= initial_health() + 1;
    free[nxt.i][nxt.j] = false;
    execute(Command(id, d));
  }

  // Reassigns the orks
  void reassign() { orkStatus.clear(); }

  // Adjusts the strategy
  void refactor(const intV& orcos) {
    // cerr << "state, call rd" << round() << endl;
    if (round() % 10 == 0) {
      // Let's reorganitzate!
      int lastState = gameState;
      switch (gameState) {
        case ANSIAROTA:
          if (winning()) gameState = DEFAULT;
          if (pctOrcos() < 90 / nb_players()) gameState = KILL;
          break;
        case KILL:
          if (pctOrcos() >= (110 / nb_players())) gameState = DEFAULT;
          break;
        case DEFAULT:
        default:

          if (not winning()) gameState = ANSIAROTA;
          if (pctOrcos() < 90 / nb_players()) gameState = KILL;

          break;
      }
      actualProbs = getProbs(gameState);
      // Canvi d'estat, reassignem orcs
      if (gameState != lastState) {
        reassign();
        // cerr << me() << "_state: cahnge from " << lastState << " to "
        //     << gameState << " on " << round() << endl;
        return;
      }
      // Han matat/ hi ha nous, reassignem per a garantir els %
      if (orcos.size() != nOrks) {
        nOrks = orcos.size();
        reassign();
      }
    }
  }
  // Basic low power mode checking
  void power_check() {
    if (winning())
      LPMODE = status(me()) > 0.4;
    else
      LPMODE = status(me()) > 0.7;
  }
  /**
   * Play method, invoked once per each round.
   */
  virtual void play() {
    // Algo-opti code
    // if (round() == 199) cerr << me() << "_state: usage=" << total_pass <<
    // endl;

    // Hem gastat massa cpu, parem abans de que el jutge ens mati
    if (status(me()) > 0.95) return;
    // Sobrao
    if (winning() and pctOrcos() > 90) return;
    power_check();

    getEnemys();
    // printMat(enemys);
    //*******************************************
    free = boolMat(rows(), boolV(cols(), true));
    intV m_orcos = orks(me());
    if (round() == 0) {
      // First round code
      nOrks = m_orcos.size();
    }

    refactor(m_orcos);

    for (int id : m_orcos) {
      move(id);
    }
  }
};

/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);
