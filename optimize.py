import threading
import operator
import subprocess
import fileinput
from random import randint, sample, shuffle
from shutil import copyfile

NUM_CORES = 32
NUM_JUGADORS = NUM_CORES * 4

MAX_POND = 5000
MIN_POND = 1

nextround = []


def gVal():
    return randint(MIN_POND, MAX_POND)


def partida(a, b, c, d):
    seed = randint(0, 2147483647)
    game = subprocess.run(["./optigame.sh", a, b, c, d, str(seed)],
                          stdout=subprocess.PIPE)
    order = {}
    for el in game.stdout.decode('UTF-8').split("\n")[:-1]:
        res = el.split()
        order[res[2]] = int(res[5])
    res = []
    for el, val in sorted(order.items(), key=operator.itemgetter(1)):
        res.append(el)

    return res


def worker(jugadors, x):
    global nextround
    res = partida(jugadors[x * 4], jugadors[(x * 4) + 1],
                  jugadors[(x * 4) + 2], jugadors[(x * 4) + 3])
    # print(res)
    r = res[:2]
    try:
        r.remove("Dummy")
    except ValueError:
        pass  # do nothing!

    nextround.extend(r)


def ronda(jugad):
    global nextround
    jugadors = jugad[:]
    while len(jugadors) % 4 != 0:

        jugadors.append("Dummy")

    nextround = []
    threads = []
    # print("Inici ronda amb ", len(jugadors), "jugadors")
    for ind in range(0, int(len(jugadors) / 4)):
        tre = threading.Thread(target=worker, args=(jugadors, ind,))
        tre.start()
        threads.append(tre)

    # Wait for threads
    for thread in threads:
        thread.join()
    # print(nextround)
    if len(nextround) >= 4:
        return ronda(nextround)
    else:
        return nextround[0]


def sort(players):

    classif = []
    pos = 0
    while len(players) > 3:
        print("##RONDA", pos, ":", len(players), end=" ")
        p = ronda(players)
        print("Muere ", p)
        players.remove(p)
        classif.append(p)
        pos += 1
    classif.extend(players)
    return classif


def reproduce(a, b):
    return int((a + b) / 2 + randint(-50, 50))


def make():
    print("Compiling...", end="", flush=True)
    subprocess.run(["make", "-f", "SuperMakefile","-j"+str(NUM_CORES-1)],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    print(" DONE")


def clean():
    subprocess.run(["make", "-f", "SuperMakefile", "fastclean"],
                   stdout=subprocess.DEVNULL)


def createPlayer(name, peso):
    copyfile("Optimizer/TEMPLATE.cc", "Optimizer/AI" + name + ".cc")

    with fileinput.FileInput("Optimizer/AI" + name + ".cc", inplace=True,) as file:
        for line in file:
            print(line.replace("PXXXXP", str(peso)), end='')
    with fileinput.FileInput("Optimizer/AI" + name + ".cc", inplace=True, ) as file:
        for line in file:
            print(line.replace("NAMENAMENAMENAME", name), end='')


def genPlayers(vals):
    players = []
    n = 0
    for val in vals:
        players.append("Player_" + str(n))
        createPlayer("Player_" + str(n), val)
        n += 1
    return players


def round(vals):
    players = genPlayers(vals)
    make()
    players = sort(players)
    newvals = []
    for el in players:
        n = int(el.split("_")[1])
        newvals.append(vals[n])
    clean()
    return newvals


def main():
    # players = ["FeartheSugus", "Rocher5", "MastP_1", "Sugus_v1", "FeartheSugus", "Rocher5",
     #          "Rocher6", "Rocher5", "FeartheSugus", "Rocher5", "Rocher6", "Sugus_v1_5",
     #          "FeartheSugus", "Rocher5", "Rocher6"]

    vals = []
    for pos in range(0, NUM_JUGADORS):
        vals.append(gVal())

    for i in range(0, 100):
        shuffle(vals)
        nv = round(vals)
        print(nv)
        best = nv[-10:]
        nous = []
        for num in range(0, int(NUM_JUGADORS * 0.8)):
            [a, b] = sample(best, 2)
            nous.append(reproduce(a, b))

        nous.extend(sample(nv, int(NUM_JUGADORS * 0.1)))

        for num in range(0, int(NUM_JUGADORS * 0.1)):
            nous.append(gVal())
        # print(nous)


if __name__ == '__main__':
    if NUM_JUGADORS < 15:
        print("I need moar cores pls")
        exit()
    main()
