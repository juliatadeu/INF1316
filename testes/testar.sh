#!/bin/bash

# Limpa arquivos antigos
rm -f acessos/acessos_P*

# Garante que o diretório existe
mkdir -p acessos

# Gera arquivos de acesso com valores previsíveis (6 acessos por processo)
for i in {1..4}; do
  file="acessos/acessos_P$i"
  for j in {0..5}; do
    echo "$((j + (i - 1) * 8)) $([ $((j % 2)) -eq 0 ] && echo R || echo W)" >> "$file"
  done
done

# Compila
gcc main.c gmv.c -o simulador || { echo "Erro ao compilar"; exit 1; }

# Executa e salva saída
./simulador > saida_teste.txt

echo "=== Teste executado ==="
cat saida_teste.txt
