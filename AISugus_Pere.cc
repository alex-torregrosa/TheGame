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
  // Test
  // int total_pass = 0;

  // Data functions
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
        enemys[i][j] = max(enemys[i][j],ork.health);
        if (i > 0) enemys[i - 1][j] = max(enemys[i-1][j],ork.health);
        if (j > 0) enemys[i][j - 1] = max(enemys[i][j-1],ork.health);
        if (i < rows() - 1) enemys[i + 1][j] = max(enemys[i+1][j],ork.health);
        if (j < cols() - 1) enemys[i][j + 1] = max(enemys[i][j+1],ork.health);
        
        
        //fase 2
        if (i > 1) enemys[i - 2][j] = max(enemys[i-2][j],ork.health);
        if (j > 1) enemys[i][j - 2] = max(enemys[i][j-2],ork.health);
        if (i < rows() - 2) enemys[i + 2][j] = max(enemys[i+2][j],ork.health);
        if (j < cols() - 2) enemys[i][j + 2] = max(enemys[i][j+2],ork.health);
      } else
        if(enemys[i][j] == -1 ) enemys[i][j] = initial_health() + 1;
        else enemys[i][j] += initial_health() + 1;
    }
  }

  // Returrns percentage of my orks (0-100)
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
    for (int d = 0; d != DIR_SIZE; ++d) {
      Dir dir = Dir(d);
      Pos npos = pos + dir;
      if (pos_ok(npos) and cell(npos).type != WATER) {
        res.push_back(npos);
      }
    }
  }

  bool hasBeefyEnemy(Pos p, Unit myUnit) {
    
      if(enemys[p.i][p.j] > myUnit.health) return true;
      else return false;
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
        if (p != pos and hasBeefyEnemy(p, u))
          visited[p.i][p.j] = true;
        else if (cmp_search(ct, c, u))
          found = true;
        else {
          visited[p.i][p.j] = true;
          posV res;
          veins(p, res, u);
          for (Pos v : res) {
            int c = 1 + 8 * cost(cell(v.i, v.j).type);
            if (prices[v.i][v.j] == -1 or
                prices[v.i][v.j] > prices[p.i][p.j] + c) {
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
       /*Hecto*/break;  // Aaaargh!!!!!! (TODO: pensar algo mejor)
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

  Dir behavior_killer(Unit u) {
    // TODO suicide if low health
    return dijkstra(u.pos, CMP_ENEMY, u).first;
  }
  /**
   * Moves the player, also state machine selection
   */
  void move(int id) {
    Unit u = unit(id);
    int s = orkStatus[id];
    Dir d;
    if (s == ORK_DEFAULT) {
      // Random initial status
      s = orkStatus[id] = (*actualProbs)[random(0, 9)];
      // cerr << "state: new guy in town: " << id << endl;
    }
    if (s == ANSIAROTA_C) {
      if (round() % 2 == 0 or not LPMODE) {
        pair<Dir, Dir> dv = dijkstra(u.pos, CMP_CITY, u);
        d = dv.first;
        nextPos[id] = dv.second;
      } else
        d = nextPos[id];
    }

    if (s == KILLER) d = behavior_killer(u);
    
    Pos nxt = u.pos +d;
    enemys[u.pos.i][u.pos.j] -= initial_health() + 1;
    enemys[nxt.i][nxt.j] += initial_health() + 1;
    execute(Command(id, d));
    
  }

  void reassign() { orkStatus.clear(); }

  void refactor() {
    // cerr << "state, call rd" << round() << endl;
    if (round() % 10 == 0) {
      // Let's reorganitzate!
      int lastState = gameState;
      switch (gameState) {
        case ANSIAROTA:
          if (winning()) gameState = DEFAULT;
          if (pctOrcos() < 100 / nb_players()) gameState = KILL;
          break;
        case KILL:
          if (pctOrcos() > 1.2 * (100 / nb_players())) gameState = DEFAULT;
          break;
        case DEFAULT:
        default:

          if (not winning()) gameState = ANSIAROTA;
          if (pctOrcos() < 100 / nb_players()) gameState = KILL;

          break;
      }
      actualProbs = getProbs(gameState);
      if (gameState != lastState) {
        reassign();
         cerr << me() << "_state: cahnge from " << lastState << " to "
             << gameState << " on " << round() << endl;
      }
    }
  }

  void power_check() {
    // double max_pct = 1 / nb_rounds();
    // double act_pct = (1 - status(me())) / (nb_rounds() - round());
    if (winning())
      LPMODE = status(me()) > 0.4;
    else
      LPMODE = status(me()) > 0.7;
  }
  /**
   * Play method, invoked once per each round.
   */
  virtual void play() {
    // if (round() == 199) cerr << me() << "_state: usage=" << total_pass <<
    // endl;

    // Hem gastat massa cpu, parem abans de que el jutge ens mati
    if (status(me()) > 0.95) return;
    // Sobrao
    if (winning() and pctOrcos() > 90) return;
    power_check();

    getEnemys();

    intV m_orcos = orks(me());
    if (round() == 0) {
      // First round code
    }

    refactor();

    for (int id : m_orcos) {
      move(id);
    }
  }
};

/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);
