"""Game Tester"""
import subprocess
from random import randint
import sys


def partida(me, other):
    seed = randint(0, 2147483647)
    game = subprocess.run(
        ["./game.sh",  me, other, str(seed)], stdout=subprocess.PIPE)
    res = str(game.stdout).split()[2]
    if res != me:
        print()
        print("Partida #", seed, "contra", other, "perdida")
        return False
    else:
        return True


me = "Sugus_Pere"

list = str(subprocess.run(["./Game", "-l"], stdout=subprocess.PIPE).stdout)
list = list[2:-3].split("\\n")
# print(list)

found = False
for el in list:
    if el == me:
        found = True

if not found:
    print("Player not found")
    exit()

print("Starting tests")


for el in list:
    if el == "Dummy":
        print("Jugando contra", el, end=" ", flush=True)
        for x in range(0, 100):
            print(".", end="", flush=True)
            if not partida(me, el):
                # exit()
                pass
        print()
print("Todas las partidas ganadas!")
