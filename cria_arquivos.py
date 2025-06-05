import random

def gerar_acessos(nome_arquivo):
    with open(nome_arquivo, "w") as f:
        for _ in range(100):
            pagina = random.randint(0, 31)
            tipo = random.choice(["R", "W"])
            f.write(f"{pagina:02} {tipo}\n")

for i in range(1, 5):
    gerar_acessos(f"acessos_P{i}")
