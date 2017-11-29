#include "Player.hh"
// Search comparisons
enum cmpSearch { CMP_CITY, CMP_ENEMY };

pair<Dir, Dir> dijkstra(const Pos& pos, cmpSearch ct, const Unit& u) {
  intMat prices(rows(), intV(cols(), -1));
  posMat parents(rows(), posV(cols()));
  boolMat visited(rows(), boolV(cols(), false));
  PosPQ pq;
  pq.push(dPos(pos, 0));

  Pos p = pos;
  while (not found and not pq.empty()) {
    p = pq.top().p;
    pq.pop();
    if (not visited[p.i][p.j]) {
      if (p != pos and hasBeefyEnemy(c, u))
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
      break;  // Aaaargh!!!!!! (TODO: pensar algo mejor)
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
