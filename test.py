"""Game Tester"""
import subprocess
from random import randint
import threading

N_THREADS = 32
N_PART = 100


perdidas = 0


def partida(me, other):
    seed = randint(0, 2147483647)
    game = subprocess.run(
        ["./game.sh",  me, other, str(seed)], stdout=subprocess.PIPE)
    res = str(game.stdout).split()[2]
    if res != me:
        print("-", end="", flush=True)
        # print()
        # print("Partida #", seed, "contra", other, "perdida")
        return False
    else:
        print(".", end="", flush=True)
        return True


def worker(enemy):
    size = int(N_PART / N_THREADS)
    global perdidas
    for x in range(0, size):
        if not partida(me, enemy):
            # exit()
            perdidas += 1


me = "FeartheSugus"

list = str(subprocess.run(["./Game", "-l"], stdout=subprocess.PIPE).stdout)
list = list[2:-3].split("\\n")
# print(list)

found = False
for el in list:
    if el == me:
        found = True

if not found:
    print("Player", me, "not found")
    exit()

print("Starting tests as", me)


for el in list:
    if el != me:
        print("Jugando contra", el, end=" ", flush=True)
        perdidas = 0
        m_threads = []
        for x in range(0, N_THREADS):
            t = threading.Thread(target=worker, args=(el,))
            t.start()
            m_threads.append(t)

        for tr in m_threads:
            tr.join()
        print()
        print(N_PART - perdidas, " partidas ganadas!")
