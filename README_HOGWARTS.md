# PAC-MAN HOGWARTS em C

Versão temática do projeto MVC em C, usando BST, AVL, grafos, BFS, DFS, Dijkstra e 7 algoritmos de ordenação.

## Compilar
```bash
make
./pacman
```

## Testes
```bash
make test
```

## Melhorias adicionadas
- Ranking persistente em `hogwarts_scores.dat`.
- Nome do jogador/bruxo com até 15 caracteres antes do jogo.
- BST com inserção, busca, remoção, altura, contagem, mínimo, máximo e ASCII art na tela de scores.
- AVL com rotações simples/duplas e contadores exibidos no painel.
- Grafos com matriz + lista de adjacência, BFS, DFS e Dijkstra.
- Tela de ranking com benchmark dos 7 sorts em microssegundos.
- Modo debug com tecla `D`, exibindo distâncias BFS sobre o mapa.
- Relatório final ao sair.
- Arquivo `test.c` com validação PASS/FAIL para BST, AVL e sorts.
- Tema visual Hogwarts: nomes, tela de ranking e painel lateral adaptados.

## Observação Windows
Prefira rodar no Windows Terminal/PowerShell com MinGW ou no WSL. O terminal antigo pode bugar ANSI.
