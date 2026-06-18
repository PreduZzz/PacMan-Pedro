/*
 * ============================================================
 *  MODEL — Logica do jogo e estruturas de dados
 * ============================================================
 */

#include "pacman.h"

static void *xmalloc(size_t size, const char *where) {
    void *p = malloc(size);
    if (!p) { fprintf(stderr, "Erro de memoria em %s. Saida limpa.\n", where); exit(EXIT_FAILURE); }
    return p;
}
static void *xcalloc(size_t n, size_t size, const char *where) {
    void *p = calloc(n, size);
    if (!p) { fprintf(stderr, "Erro de memoria em %s. Saida limpa.\n", where); exit(EXIT_FAILURE); }
    return p;
}

/* ============================================================
 *  MAPA CLASSICO DO PAC-MAN HOGWARTS (28x31)
 * ============================================================ */

static const char *raw_map[MAP_H] = {
    "############################",
    "#............##............#",
    "#.####.#####.##.#####.####.#",
    "#o####.#####.##.#####.####o#",
    "#.####.#####.##.#####.####.#",
    "#..........................#",
    "#.####.##.########.##.####.#",
    "#.####.##.########.##.####.#",
    "#......##....##....##......#",
    "######.##### ## #####.######",
    "     #.##### ## #####.#     ",
    "     #.##          ##.#     ",
    "     #.## ###--### ##.#     ",
    "######.## #GGGGGG# ##.######",
    "T     .   #GGGGGG#   .     T",
    "######.## #GGGGGG# ##.######",
    "     #.## ######## ##.#     ",
    "     #.##          ##.#     ",
    "     #.## ######## ##.#     ",
    "######.## ######## ##.######",
    "#............##............#",
    "#.####.#####.##.#####.####.#",
    "#.####.#####.##.#####.####.#",
    "#o..##.......  .......##..o#",
    "###.##.##.########.##.##.###",
    "###.##.##.########.##.##.###",
    "#......##....##....##......#",
    "#.##########.##.##########.#",
    "#.##########.##.##########.#",
    "#..........................#",
    "############################"
};

static const char *map_names[MAP_COUNT] = {
    "Classico",
    "Arena aberta",
    "Labirinto duro",
    "Corredores de Hogwarts",
    "Camara Secreta"
};

const char *model_map_name(int index) {
    if (index < 0 || index >= MAP_COUNT) return map_names[0];
    return map_names[index];
}

static void map_set_tile(GameModel *m, int x, int y, int tile) {
    if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H) return;
    m->grid[y][x] = tile;
}

static void model_apply_map_variant(GameModel *m) {
    if (m->map_selected == 1) {
        /* Arena mais aberta: corredores centrais extras e rotas laterais. */
        for (int y = 2; y < MAP_H - 2; y++) {
            if (y >= 11 && y <= 17) continue;
            map_set_tile(m, 13, y, TILE_PELLET);
            map_set_tile(m, 14, y, TILE_PELLET);
        }
        for (int x = 2; x < MAP_W - 2; x++) {
            if (x >= 11 && x <= 16) continue;
            map_set_tile(m, x, 17, TILE_PELLET);
        }
        map_set_tile(m, 1, 1, TILE_POWER);
        map_set_tile(m, 26, 1, TILE_POWER);
        map_set_tile(m, 1, 29, TILE_POWER);
        map_set_tile(m, 26, 29, TILE_POWER);
    } else if (m->map_selected == 2) {
        /* Tuneis multiplos: corredores horizontais e verticais abertos. */
        /* Cria 3 corredores horizontais principais */
        for (int x = 2; x < MAP_W - 2; x++) {
            if (x >= 11 && x <= 16) continue;
            map_set_tile(m, x, 8, TILE_PELLET);
            map_set_tile(m, x, 15, TILE_PELLET);
            map_set_tile(m, x, 22, TILE_PELLET);
        }
        /* Cria 4 corredores verticais para conectar */
        for (int y = 2; y < MAP_H - 2; y++) {
            map_set_tile(m, 7, y, TILE_PELLET);
            map_set_tile(m, 13, y, TILE_PELLET);
            map_set_tile(m, 14, y, TILE_PELLET);
            map_set_tile(m, 20, y, TILE_PELLET);
        }
        /* Power pellets nos cantos */
        map_set_tile(m, 2, 2, TILE_POWER);
        map_set_tile(m, 25, 2, TILE_POWER);
        map_set_tile(m, 2, 28, TILE_POWER);
        map_set_tile(m, 25, 28, TILE_POWER);
      } else if (m->map_selected == 3) {
        /* Corredores de Hogwarts */
        for (int y = 2; y < MAP_H - 2; y++) {
            if (y % 4 == 0) {
                for (int x = 2; x < MAP_W - 2; x++) {
                    if (x < 11 || x > 16)
                        map_set_tile(m, x, y, TILE_PELLET);
                }
            }
        }

        for (int x = 4; x < MAP_W - 4; x += 6) {
            for (int y = 3; y < MAP_H - 3; y++) {
                if (y < 11 || y > 17)
                    map_set_tile(m, x, y, TILE_PELLET);
            }
        }

        map_set_tile(m, 2, 2, TILE_POWER);
        map_set_tile(m, 25, 2, TILE_POWER);
        map_set_tile(m, 2, 28, TILE_POWER);
        map_set_tile(m, 25, 28, TILE_POWER);

    } else if (m->map_selected == 4) {
    /* Camara Secreta: dificil, mas sempre acessivel */

    /* Abre corredores principais */
    for (int x = 1; x < MAP_W - 1; x++) {
        map_set_tile(m, x, 5, TILE_PELLET);
        map_set_tile(m, x, 14, TILE_PELLET);
        map_set_tile(m, x, 23, TILE_PELLET);
        map_set_tile(m, x, 29, TILE_PELLET);
    }

    for (int y = 1; y < MAP_H - 1; y++) {
        map_set_tile(m, 3, y, TILE_PELLET);
        map_set_tile(m, 10, y, TILE_PELLET);
        map_set_tile(m, 17, y, TILE_PELLET);
        map_set_tile(m, 24, y, TILE_PELLET);
    }

    /* Obstaculos pequenos, sem fechar tudo */
    for (int y = 7; y < MAP_H - 7; y += 6) {
        for (int x = 6; x < MAP_W - 6; x += 8) {
            map_set_tile(m, x, y, TILE_WALL);
            map_set_tile(m, x + 1, y, TILE_WALL);
        }
    }

    /* Mantem area central dos fantasmas livre */
    for (int y = 11; y <= 17; y++) {
        for (int x = 11; x <= 16; x++) {
            map_set_tile(m, x, y, TILE_EMPTY);
        }
    }

    /* Power pellets acessiveis */
    map_set_tile(m, 1, 3, TILE_POWER);
    map_set_tile(m, 26, 3, TILE_POWER);
    map_set_tile(m, 1, 27, TILE_POWER);
    map_set_tile(m, 26, 27, TILE_POWER);
}
}

static void model_recount_pellets(GameModel *m) {
    m->pellets_left = 0;
    m->total_pellets = 0;
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            if (m->grid[y][x] == TILE_PELLET || m->grid[y][x] == TILE_POWER) {
                m->pellets_left++;
                m->total_pellets++;
            }
        }
    }
}

/* ============================================================
 *  BST — Arvore Binaria de Busca (High Scores)
 * ============================================================ */

BSTNode *bst_create_node(int score, const char *name) {
    BSTNode *n = (BSTNode *)xmalloc(sizeof(BSTNode), "bst_create_node");
    n->score = score;
    strncpy(n->name, name, 15);
    n->name[15] = '\0';
    n->left = n->right = NULL;
    return n;
}

BSTNode *bst_insert(BSTNode *root, int score, const char *name) {
    if (!root) return bst_create_node(score, name);
    if (score < root->score)
        root->left = bst_insert(root->left, score, name);
    else
        root->right = bst_insert(root->right, score, name);
    /* Duplicatas vão para a direita */
    return root;
}

BSTNode *bst_search(BSTNode *root, int score) {
    if (!root || root->score == score) return root;
    if (score < root->score) return bst_search(root->left, score);
    return bst_search(root->right, score);
}

BSTNode *bst_min(BSTNode *root) {
    if (!root) return NULL;
    while (root->left) root = root->left;
    return root;
}

BSTNode *bst_max(BSTNode *root) {
    if (!root) return NULL;
    while (root->right) root = root->right;
    return root;
}

BSTNode *bst_remove(BSTNode *root, int score) {
    if (!root) return NULL;
    if (score < root->score) {
        root->left = bst_remove(root->left, score);
    } else if (score > root->score) {
        root->right = bst_remove(root->right, score);
    } else {
        /* No encontrado */
        if (!root->left) {
            BSTNode *tmp = root->right;
            free(root);
            return tmp;
        }
        if (!root->right) {
            BSTNode *tmp = root->left;
            free(root);
            return tmp;
        }
        /* Dois filhos: pega o sucessor in-order */
        BSTNode *succ = bst_min(root->right);
        root->score = succ->score;
        strncpy(root->name, succ->name, 15);
        root->right = bst_remove(root->right, succ->score);
    }
    return root;
}

int bst_height(BSTNode *root) {
    if (!root) return 0;
    int lh = bst_height(root->left);
    int rh = bst_height(root->right);
    return 1 + (lh > rh ? lh : rh);
}

int bst_count(BSTNode *root) {
    if (!root) return 0;
    return 1 + bst_count(root->left) + bst_count(root->right);
}

/* Travessia in-order reversa (maior para menor) */
void bst_inorder_rev(BSTNode *root, int *arr, char names[][16], int *count, int max) {
    if (!root || *count >= max) return;
    bst_inorder_rev(root->right, arr, names, count, max);
    if (*count < max) {
        arr[*count] = root->score;
        strncpy(names[*count], root->name, 15);
        names[*count][15] = '\0';
        (*count)++;
    }
    bst_inorder_rev(root->left, arr, names, count, max);
}

void bst_inorder(BSTNode *root, int *arr, int *count, int max) {
    if (!root || *count >= max) return;
    bst_inorder(root->left, arr, count, max);
    if (*count < max) {
        arr[*count] = root->score;
        (*count)++;
    }
    bst_inorder(root->right, arr, count, max);
}

void bst_free(BSTNode *root) {
    /* Complexidade O(n): libera cada no uma vez. */
    if (!root) return;
    bst_free(root->left);
    bst_free(root->right);
    free(root);
}

void bst_print_ascii(BSTNode *root, char lines[][96], int *line, int max_lines, int depth) {
    /* Complexidade O(n): visita cada no da BST uma vez. */
    if (!root || *line >= max_lines) return;
    bst_print_ascii(root->right, lines, line, max_lines, depth + 1);
    if (*line < max_lines) {
        snprintf(lines[*line], 96, "%*s%d (%s)", depth * 4, "", root->score, root->name);
        (*line)++;
    }
    bst_print_ascii(root->left, lines, line, max_lines, depth + 1);
}

static int bst_save_node(FILE *f, BSTNode *root) {
    /* Complexidade O(n): salva todos os nos em pre-ordem. */
    if (!root) return 1;
    if (fwrite(&root->score, sizeof(int), 1, f) != 1) return 0;
    if (fwrite(root->name, sizeof(char), 16, f) != 16) return 0;
    return bst_save_node(f, root->left) && bst_save_node(f, root->right);
}

int bst_save_binary(BSTNode *root, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return 0;
    int count = bst_count(root);
    fwrite(&count, sizeof(int), 1, f);
    int ok = bst_save_node(f, root);
    fclose(f);
    return ok;
}

int bst_load_binary(BSTNode **root, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return 0;
    int count = 0, score = 0;
    char name[16];
    if (fread(&count, sizeof(int), 1, f) != 1) { fclose(f); return 0; }
    for (int i = 0; i < count; i++) {
        if (fread(&score, sizeof(int), 1, f) != 1) break;
        if (fread(name, sizeof(char), 16, f) != 16) break;
        name[15] = '\0';
        *root = bst_insert(*root, score, name);
    }
    fclose(f);
    return 1;
}

/* ============================================================
 *  AVL — Arvore Balanceada (Inventario de Power-ups)
 * ============================================================ */

int avl_height(AVLNode *node) {
    return node ? node->height : 0;
}

static int avl_max(int a, int b) { return a > b ? a : b; }
static int g_avl_single_rot = 0;
static int g_avl_double_rot = 0;
int avl_rotation_single_count(void) { return g_avl_single_rot; }
int avl_rotation_double_count(void) { return g_avl_double_rot; }

int avl_balance_factor(AVLNode *node) {
    if (!node) return 0;
    return avl_height(node->left) - avl_height(node->right);
}

static void avl_update_height(AVLNode *node) {
    if (node) node->height = 1 + avl_max(avl_height(node->left), avl_height(node->right));
}

/* Rotacao simples a direita */
AVLNode *avl_rotate_right(AVLNode *y) {
    /* Complexidade O(1): rotacao simples a direita. */
    g_avl_single_rot++;
    AVLNode *x = y->left;
    AVLNode *T2 = x->right;
    x->right = y;
    y->left = T2;
    avl_update_height(y);
    avl_update_height(x);
    return x;
}

/* Rotacao simples a esquerda */
AVLNode *avl_rotate_left(AVLNode *x) {
    /* Complexidade O(1): rotacao simples a esquerda. */
    g_avl_single_rot++;
    AVLNode *y = x->right;
    AVLNode *T2 = y->left;
    y->left = x;
    x->right = T2;
    avl_update_height(x);
    avl_update_height(y);
    return y;
}

/* Rotacao dupla esquerda-direita */
AVLNode *avl_rotate_left_right(AVLNode *node) {
    g_avl_double_rot++;
    node->left = avl_rotate_left(node->left);
    return avl_rotate_right(node);
}

/* Rotacao dupla direita-esquerda */
AVLNode *avl_rotate_right_left(AVLNode *node) {
    g_avl_double_rot++;
    node->right = avl_rotate_right(node->right);
    return avl_rotate_left(node);
}

static AVLNode *avl_balance(AVLNode *node) {
    if (!node) return NULL;
    avl_update_height(node);
    int bf = avl_balance_factor(node);

    if (bf > 1) {
        if (avl_balance_factor(node->left) >= 0)
            return avl_rotate_right(node);          /* Rotacao simples direita */
        else
            return avl_rotate_left_right(node);     /* Rotacao dupla esquerda-direita */
    }
    if (bf < -1) {
        if (avl_balance_factor(node->right) <= 0)
            return avl_rotate_left(node);           /* Rotacao simples esquerda */
        else
            return avl_rotate_right_left(node);     /* Rotacao dupla direita-esquerda */
    }
    return node;
}

AVLNode *avl_insert(AVLNode *root, int key, int value) {
    if (!root) {
        AVLNode *n = (AVLNode *)xmalloc(sizeof(AVLNode), "avl_insert");
        n->key = key; n->value = value; n->height = 1;
        n->left = n->right = NULL;
        return n;
    }
    if (key < root->key)
        root->left = avl_insert(root->left, key, value);
    else if (key > root->key)
        root->right = avl_insert(root->right, key, value);
    else
        root->value += value;  /* Acumula quantidade */

    return avl_balance(root);
}

AVLNode *avl_search(AVLNode *root, int key) {
    if (!root || root->key == key) return root;
    if (key < root->key) return avl_search(root->left, key);
    return avl_search(root->right, key);
}

AVLNode *avl_remove(AVLNode *root, int key) {
    if (!root) return NULL;
    if (key < root->key) {
        root->left = avl_remove(root->left, key);
    } else if (key > root->key) {
        root->right = avl_remove(root->right, key);
    } else {
        if (!root->left || !root->right) {
            AVLNode *tmp = root->left ? root->left : root->right;
            free(root);
            return tmp;
        }
        /* Sucessor in-order */
        AVLNode *succ = root->right;
        while (succ->left) succ = succ->left;
        root->key = succ->key;
        root->value = succ->value;
        root->right = avl_remove(root->right, succ->key);
    }
    return avl_balance(root);
}

void avl_free(AVLNode *root) {
    if (!root) return;
    avl_free(root->left);
    avl_free(root->right);
    free(root);
}

/* ============================================================
 *  GRAFO — Representacao do Labirinto
 * ============================================================ */

int model_coord_to_vertex(int x, int y) {
    return y * MAP_W + x;
}

void model_vertex_to_coord(int v, int *x, int *y) {
    *x = v % MAP_W;
    *y = v / MAP_W;
}

Graph *graph_create(int num_vertices) {
    Graph *g = (Graph *)xmalloc(sizeof(Graph), "graph_create");
    g->num_vertices = num_vertices;

    /* Lista de adjacencias */
    g->adj_list = (AdjNode **)xcalloc(num_vertices, sizeof(AdjNode *), "graph_adj_list");
    if (!g->adj_list) { free(g); return NULL; }

    /* Matriz de adjacencias */
    g->adj_matrix = (int *)xcalloc(num_vertices * num_vertices, sizeof(int), "graph_adj_matrix");
    if (!g->adj_matrix) { free(g->adj_list); free(g); return NULL; }
    g->matrix_allocated = 1;

    return g;
}

void graph_add_edge(Graph *g, int u, int v, int weight) {
    /* Lista de adjacencias */
    AdjNode *node = (AdjNode *)malloc(sizeof(AdjNode));
    node->vertex = v;
    node->weight = weight;
    node->next = g->adj_list[u];
    g->adj_list[u] = node;

    /* Matriz de adjacencias (bidirecional) */
    g->adj_matrix[u * g->num_vertices + v] = weight;
    g->adj_matrix[v * g->num_vertices + u] = weight;
}

int graph_has_edge_matrix(Graph *g, int u, int v) {
    return g->adj_matrix[u * g->num_vertices + v] > 0;
}

int graph_has_edge_list(Graph *g, int u, int v) {
    AdjNode *cur = g->adj_list[u];
    while (cur) {
        if (cur->vertex == v) return 1;
        cur = cur->next;
    }
    return 0;
}

void graph_build_from_map(Graph *g, int grid[MAP_H][MAP_W]) {
    int dirs[4][2] = {{0,-1},{0,1},{-1,0},{1,0}};
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            if (grid[y][x] == TILE_WALL) continue;
            int u = model_coord_to_vertex(x, y);
            for (int d = 0; d < 4; d++) {
                int nx = x + dirs[d][0];
                int ny = y + dirs[d][1];

                /* Tunel wrap */
                if (nx < 0) nx = MAP_W - 1;
                if (nx >= MAP_W) nx = 0;

                if (ny < 0 || ny >= MAP_H) continue;
                if (grid[ny][nx] == TILE_WALL) continue;
                if (grid[ny][nx] == TILE_DOOR) continue;

                int v = model_coord_to_vertex(nx, ny);
                int weight = 1;
                if (grid[ny][nx] == TILE_TUNNEL) weight = 2; /* Tunel mais lento */

                /* Evita duplicatas na lista */
                if (!graph_has_edge_list(g, u, v))
                    graph_add_edge(g, u, v, weight);
            }
        }
    }
}

void graph_free(Graph *g) {
    if (!g) return;
    for (int i = 0; i < g->num_vertices; i++) {
        AdjNode *cur = g->adj_list[i];
        while (cur) {
            AdjNode *tmp = cur;
            cur = cur->next;
            free(tmp);
        }
    }
    free(g->adj_list);
    if (g->matrix_allocated) free(g->adj_matrix);
    free(g);
}

/* ============================================================
 *  QUEUE — Fila para BFS
 * ============================================================ */

Queue *queue_create(void) {
    Queue *q = (Queue *)malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    q->size = 0;
    return q;
}

void queue_push(Queue *q, int data) {
    QueueNode *n = (QueueNode *)malloc(sizeof(QueueNode));
    n->data = data;
    n->next = NULL;
    if (q->rear) q->rear->next = n;
    else q->front = n;
    q->rear = n;
    q->size++;
}

int queue_pop(Queue *q) {
    if (!q->front) return -1;
    QueueNode *tmp = q->front;
    int data = tmp->data;
    q->front = tmp->next;
    if (!q->front) q->rear = NULL;
    free(tmp);
    q->size--;
    return data;
}

int queue_empty(Queue *q) { return q->size == 0; }

void queue_free(Queue *q) {
    while (!queue_empty(q)) queue_pop(q);
    free(q);
}

/* ============================================================
 *  MIN-HEAP — Fila de prioridade para Dijkstra
 * ============================================================ */

MinHeap *heap_create(int capacity) {
    MinHeap *h = (MinHeap *)xmalloc(sizeof(MinHeap), "heap_create");
    h->items = (HeapItem *)xmalloc(sizeof(HeapItem) * capacity, "heap_items");
    h->size = 0;
    h->capacity = capacity;
    return h;
}

static void heap_swap(HeapItem *a, HeapItem *b) {
    HeapItem tmp = *a; *a = *b; *b = tmp;
}

void heap_push(MinHeap *h, int vertex, int dist) {
    if (h->size >= h->capacity) return;
    h->items[h->size] = (HeapItem){vertex, dist};
    int i = h->size++;
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->items[parent].dist > h->items[i].dist) {
            heap_swap(&h->items[parent], &h->items[i]);
            i = parent;
        } else break;
    }
}

HeapItem heap_pop(MinHeap *h) {
    HeapItem top = h->items[0];
    h->items[0] = h->items[--h->size];
    int i = 0;
    while (1) {
        int smallest = i;
        int l = 2 * i + 1, r = 2 * i + 2;
        if (l < h->size && h->items[l].dist < h->items[smallest].dist) smallest = l;
        if (r < h->size && h->items[r].dist < h->items[smallest].dist) smallest = r;
        if (smallest != i) { heap_swap(&h->items[i], &h->items[smallest]); i = smallest; }
        else break;
    }
    return top;
}

int heap_empty(MinHeap *h) { return h->size == 0; }

void heap_free(MinHeap *h) {
    free(h->items);
    free(h);
}

/* ============================================================
 *  BFS — Busca em Largura (IA do Pinky)
 * ============================================================ */

void bfs(Graph *g, int src, int *dist, int *prev) {
    int n = g->num_vertices;
    for (int i = 0; i < n; i++) { dist[i] = INF; prev[i] = -1; }
    dist[src] = 0;

    Queue *q = queue_create();
    queue_push(q, src);

    while (!queue_empty(q)) {
        int u = queue_pop(q);
        AdjNode *adj = g->adj_list[u];
        while (adj) {
            int v = adj->vertex;
            if (dist[v] == INF) {
                dist[v] = dist[u] + 1;
                prev[v] = u;
                queue_push(q, v);
            }
            adj = adj->next;
        }
    }
    queue_free(q);
}

/* ============================================================
 *  DFS — Busca em Profundidade (IA do Inky)
 * ============================================================ */

static int dfs_found;

static void dfs_helper(Graph *g, int cur, int dst, int *visited, int *path, int *path_len) {
    if (dfs_found) return;
    visited[cur] = 1;
    path[(*path_len)++] = cur;

    if (cur == dst) { dfs_found = 1; return; }

    AdjNode *adj = g->adj_list[cur];
    while (adj && !dfs_found) {
        if (!visited[adj->vertex]) {
            dfs_helper(g, adj->vertex, dst, visited, path, path_len);
        }
        adj = adj->next;
    }

    if (!dfs_found) (*path_len)--;
}

void dfs(Graph *g, int src, int dst, int *visited, int *path, int *path_len) {
    int n = g->num_vertices;
    for (int i = 0; i < n; i++) visited[i] = 0;
    *path_len = 0;
    dfs_found = 0;
    dfs_helper(g, src, dst, visited, path, path_len);
}

/* ============================================================
 *  DIJKSTRA — Caminho mais curto (IA do Blinky)
 * ============================================================ */

void dijkstra(Graph *g, int src, int *dist, int *prev, int *visited) {
    int n = g->num_vertices;
    memset(visited, 0, n * sizeof(int));
    for (int i = 0; i < n; i++) { dist[i] = INF; prev[i] = -1; }
    dist[src] = 0;

    MinHeap *h = heap_create(n);
    heap_push(h, src, 0);

    while (!heap_empty(h)) {
        HeapItem top = heap_pop(h);
        int u = top.vertex;
        if (visited[u]) continue;
        visited[u] = 1;

        AdjNode *adj = g->adj_list[u];
        while (adj) {
            int v = adj->vertex;
            int w = adj->weight;
            if (!visited[v] && dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                prev[v] = u;
                heap_push(h, v, dist[v]);
            }
            adj = adj->next;
        }
    }

    heap_free(h);
}

/* ============================================================
 *  ALGORITMOS DE ORDENACAO
 * ============================================================ */

void bubble_sort(int *arr, int n) {
    for (int i = 0; i < n - 1; i++) {
        int swapped = 0;
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j] < arr[j + 1]) {  /* Decrescente */
                int tmp = arr[j]; arr[j] = arr[j + 1]; arr[j + 1] = tmp;
                swapped = 1;
            }
        }
        if (!swapped) break;
    }
}

void selection_sort(int *arr, int n) {
    for (int i = 0; i < n - 1; i++) {
        int max_idx = i;
        for (int j = i + 1; j < n; j++) {
            if (arr[j] > arr[max_idx]) max_idx = j;
        }
        if (max_idx != i) {
            int tmp = arr[i]; arr[i] = arr[max_idx]; arr[max_idx] = tmp;
        }
    }
}

void insertion_sort(int *arr, int n) {
    for (int i = 1; i < n; i++) {
        int key = arr[i], j = i - 1;
        while (j >= 0 && arr[j] < key) {  /* Decrescente */
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

void shell_sort(int *arr, int n) {
    for (int gap = n / 2; gap > 0; gap /= 2) {
        for (int i = gap; i < n; i++) {
            int temp = arr[i], j;
            for (j = i; j >= gap && arr[j - gap] < temp; j -= gap)
                arr[j] = arr[j - gap];
            arr[j] = temp;
        }
    }
}

static void merge(int *arr, int l, int m, int r) {
    int n1 = m - l + 1, n2 = r - m;
    int *L = (int *)xmalloc(n1 * sizeof(int), "merge_left");
    int *R = (int *)xmalloc(n2 * sizeof(int), "merge_right");
    for (int i = 0; i < n1; i++) L[i] = arr[l + i];
    for (int i = 0; i < n2; i++) R[i] = arr[m + 1 + i];

    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2) {
        if (L[i] >= R[j]) arr[k++] = L[i++];  /* Decrescente */
        else arr[k++] = R[j++];
    }
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];
    free(L); free(R);
}

static void merge_sort_helper(int *arr, int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        merge_sort_helper(arr, l, m);
        merge_sort_helper(arr, m + 1, r);
        merge(arr, l, m, r);
    }
}

void merge_sort(int *arr, int n) {
    if (n > 1) merge_sort_helper(arr, 0, n - 1);
}

static int qs_partition(int *arr, int low, int high) {
    int pivot = arr[high];
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (arr[j] >= pivot) {  /* Decrescente */
            i++;
            int tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp;
        }
    }
    int tmp = arr[i + 1]; arr[i + 1] = arr[high]; arr[high] = tmp;
    return i + 1;
}

void quick_sort(int *arr, int low, int high) {
    if (low < high) {
        int pi = qs_partition(arr, low, high);
        quick_sort(arr, low, pi - 1);
        quick_sort(arr, pi + 1, high);
    }
}

static void heapify_max(int *arr, int n, int i) {
    int largest = i;
    int l = 2 * i + 1, r = 2 * i + 2;
    if (l < n && arr[l] < arr[largest]) largest = l;  /* Min-heap para decrescente */
    if (r < n && arr[r] < arr[largest]) largest = r;
    if (largest != i) {
        int tmp = arr[i]; arr[i] = arr[largest]; arr[largest] = tmp;
        heapify_max(arr, n, largest);
    }
}

void heap_sort(int *arr, int n) {
    for (int i = n / 2 - 1; i >= 0; i--) heapify_max(arr, n, i);
    for (int i = n - 1; i > 0; i--) {
        int tmp = arr[0]; arr[0] = arr[i]; arr[i] = tmp;
        heapify_max(arr, i, 0);
    }
}

/* ============================================================
 *  MODEL CORE — Inicializacao e logica do jogo
 * ============================================================ */

void model_init(GameModel *m) {
    memset(m, 0, sizeof(GameModel));
    m->lives = START_LIVES;
    m->level = 1;
    m->state = STATE_MENU;
    m->running = 1;
    m->score_tree = NULL;
    m->powerup_tree = NULL;
    m->maze_graph = NULL;
    m->score_history_count = 0;
    m->bonus_timer = 0;
    m->bonus_points = 0;
    m->bonus_x = 0;
    m->bonus_y = 0;
    m->menu_selected = 0;
    m->map_selected = 0;
    m->splash_shown = 0;
    strncpy(m->player_name, "BRUXO", sizeof(m->player_name)-1);
    strcpy(m->best_sort_name, "-");
    m->best_sort_time = 0;

    model_load_scores(m);
    if (m->score_tree) {
        BSTNode *mx = bst_max(m->score_tree);
        if (mx) m->high_score = mx->score;
    } else {
        /* Pre-carrega alguns high scores na BST */
        m->score_tree = bst_insert(m->score_tree, 10000, "DUMBLE");
        m->score_tree = bst_insert(m->score_tree, 8000,  "HARRY");
        m->score_tree = bst_insert(m->score_tree, 6000,  "HERMIONE");
        m->score_tree = bst_insert(m->score_tree, 4000,  "RON");
        m->score_tree = bst_insert(m->score_tree, 2000,  "NEVILLE");
    }

    /* Power-ups iniciais na AVL */
    m->powerup_tree = avl_insert(m->powerup_tree, 1, 0);  /* Speed boost */
    m->powerup_tree = avl_insert(m->powerup_tree, 2, 0);  /* Double points */
    m->powerup_tree = avl_insert(m->powerup_tree, 3, 0);  /* Extra time */
    m->powerup_tree = avl_insert(m->powerup_tree, 4, 0);  /* Ghost freeze */
    m->avl_rot_single = avl_rotation_single_count();
    m->avl_rot_double = avl_rotation_double_count();
}

void model_load_map(GameModel *m) {
    m->pellets_left = 0;
    m->total_pellets = 0;

    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            char c = raw_map[y][x];
            switch (c) {
                case '#': m->grid[y][x] = TILE_WALL; break;
                case '.': m->grid[y][x] = TILE_PELLET; m->pellets_left++; m->total_pellets++; break;
                case 'o': m->grid[y][x] = TILE_POWER; m->pellets_left++; m->total_pellets++; break;
                case '-': m->grid[y][x] = TILE_DOOR; break;
                case 'T': m->grid[y][x] = TILE_TUNNEL; break;
                case 'G': m->grid[y][x] = TILE_EMPTY; break;
                default:  m->grid[y][x] = TILE_EMPTY; break;
            }
        }
    }
    model_apply_map_variant(m);

/* Casinha dos fantasmas fechada igual ao mapa classico */
for (int y = 11; y <= 16; y++) {
    for (int x = 10; x <= 17; x++) {
        map_set_tile(m, x, y, TILE_EMPTY);
    }
}

/* teto */
for (int x = 10; x <= 17; x++) {
    map_set_tile(m, x, 12, TILE_WALL);
}

/* porta no teto */
map_set_tile(m, 13, 12, TILE_DOOR);
map_set_tile(m, 14, 12, TILE_DOOR);

/* laterais */
for (int y = 12; y <= 16; y++) {
    map_set_tile(m, 10, y, TILE_WALL);
    map_set_tile(m, 17, y, TILE_WALL);
}

/* chão */
for (int x = 10; x <= 17; x++) {
    map_set_tile(m, x, 16, TILE_WALL);
}

/* interior livre */
for (int y = 13; y <= 15; y++) {
    for (int x = 11; x <= 16; x++) {
        map_set_tile(m, x, y, TILE_EMPTY);
    }
}
    
    model_recount_pellets(m);

    /* Pac-Man */
    m->pacman = (Entity){14, 23, 0, 0, 14, 23};

    /* Fantasmas */
    m->ghosts[0].e = (Entity){14, 11, -1, 0, 14, 11};
    m->ghosts[0].symbol = 'B'; m->ghosts[0].vulnerable = 0;
    m->ghosts[0].eaten = 0; m->ghosts[0].in_house = 0; m->ghosts[0].release_timer = 0;

    m->ghosts[1].e = (Entity){13, 14, 0, -1, 13, 14};
    m->ghosts[1].symbol = 'P'; m->ghosts[1].vulnerable = 0;
    m->ghosts[1].eaten = 0; m->ghosts[1].in_house = 1; m->ghosts[1].release_timer = 30;

    m->ghosts[2].e = (Entity){14, 14, 0, -1, 14, 14};
    m->ghosts[2].symbol = 'I'; m->ghosts[2].vulnerable = 0;
    m->ghosts[2].eaten = 0; m->ghosts[2].in_house = 1; m->ghosts[2].release_timer = 60;

    m->ghosts[3].e = (Entity){15, 14, 0, -1, 15, 14};
    m->ghosts[3].symbol = 'C'; m->ghosts[3].vulnerable = 0;
    m->ghosts[3].eaten = 0; m->ghosts[3].in_house = 1; m->ghosts[3].release_timer = 90;

    m->desired_dx = m->desired_dy = 0;
    m->current_dx = m->current_dy = 0;
    m->power_timer = 0;
    m->ghost_eat_combo = 0;
    m->frame_count = 0;
    m->ready_timer = 25;

    /* Constroi o grafo do labirinto */
    if (m->maze_graph) graph_free(m->maze_graph);
    m->maze_graph = graph_create(MAP_W * MAP_H);
    graph_build_from_map(m->maze_graph, m->grid);
}

int model_is_walkable(GameModel *m, int x, int y, int for_ghost) {
    if (x < 0 || x >= MAP_W) {
        if (y == TUNNEL_ROW) return 1;
        return 0;
    }
    if (y < 0 || y >= MAP_H) return 0;
    if (m->grid[y][x] == TILE_WALL) return 0;
    if (!for_ghost && m->grid[y][x] == TILE_DOOR) return 0;
    return 1;
}

static void wrap_entity(Entity *e) {
    if (e->x < 0) e->x = MAP_W - 1;
    if (e->x >= MAP_W) e->x = 0;
}

void model_reset_positions(GameModel *m) {
    m->pacman.x = m->pacman.start_x;
    m->pacman.y = m->pacman.start_y;
    m->current_dx = m->current_dy = 0;
    m->desired_dx = m->desired_dy = 0;

    m->ghosts[0].e = (Entity){14, 11, -1, 0, 14, 11};
    m->ghosts[1].e = (Entity){13, 14, 0, -1, 13, 14};
    m->ghosts[2].e = (Entity){14, 14, 0, -1, 14, 14};
    m->ghosts[3].e = (Entity){15, 14, 0, -1, 15, 14};

    for (int i = 0; i < GHOSTS; i++) {
        m->ghosts[i].vulnerable = 0;
        m->ghosts[i].eaten = 0;
        m->ghosts[i].in_house = (i > 0);
        m->ghosts[i].release_timer = i * 30;
    }
    m->ghosts[0].release_timer = 0;
    m->ghosts[0].in_house = 0;

    m->power_timer = 0;
    m->ghost_eat_combo = 0;
    m->ready_timer = 25;
}

void model_move_pacman(GameModel *m) {
    if (model_is_walkable(m, m->pacman.x + m->desired_dx, m->pacman.y + m->desired_dy, 0)) {
        m->current_dx = m->desired_dx;
        m->current_dy = m->desired_dy;
    }

    if (model_is_walkable(m, m->pacman.x + m->current_dx, m->pacman.y + m->current_dy, 0)) {
        m->pacman.x += m->current_dx;
        m->pacman.y += m->current_dy;
        wrap_entity(&m->pacman);
    }

    if (m->grid[m->pacman.y][m->pacman.x] == TILE_PELLET) {
        m->grid[m->pacman.y][m->pacman.x] = TILE_EMPTY;
        m->score += 10;
        m->pellets_left--;

        /* Registra score no historico para sorting demos */
        if (m->score_history_count < MAX_SCORES)
            m->score_history[m->score_history_count++] = m->score;

    } else if (m->grid[m->pacman.y][m->pacman.x] == TILE_POWER) {
        m->grid[m->pacman.y][m->pacman.x] = TILE_EMPTY;
        m->score += 50;
        m->pellets_left--;
        m->power_timer = POWER_TIME;
        m->ghost_eat_combo = 0;

        for (int i = 0; i < GHOSTS; i++) {
            if (!m->ghosts[i].eaten)
                m->ghosts[i].vulnerable = 1;
        }

        /* Usa Dijkstra para calcular distancia de cada fantasma vulneravel */
        int pacman_vertex = model_coord_to_vertex(m->pacman.x, m->pacman.y);
        dijkstra(m->maze_graph, pacman_vertex, m->dijkstra_dist, m->dijkstra_prev, m->search_visited);

        /* Coleta power-up na AVL (incrementa quantidade) com peso de distancia */
        int pu_type = (rand() % 4) + 1;
        for (int i = 0; i < GHOSTS; i++) {
            if (m->ghosts[i].vulnerable) {
                int ghost_vertex = model_coord_to_vertex(m->ghosts[i].e.x, m->ghosts[i].e.y);
                int dist = m->dijkstra_dist[ghost_vertex];
                m->powerup_tree = avl_insert(m->powerup_tree, pu_type + i, dist / 10);
                m->avl_rot_single = avl_rotation_single_count();
                m->avl_rot_double = avl_rotation_double_count();
            }
        }
    }

    if (m->score > m->high_score) m->high_score = m->score;
}

/* Usa os algoritmos de grafo para decidir a direcao dos fantasmas */
static void ghost_get_next_dir(GameModel *m, int ghost_idx, int tx, int ty, int *out_dx, int *out_dy) {
    Ghost *g = &m->ghosts[ghost_idx];
    int src = model_coord_to_vertex(g->e.x, g->e.y);
    int dst = model_coord_to_vertex(tx, ty);
    int next_vertex = -1;

    if (src == dst) { *out_dx = 0; *out_dy = 0; return; }

    if (ghost_idx == 0 && !g->vulnerable) {
        /* BLINKY: usa Dijkstra */
        dijkstra(m->maze_graph, src, m->dijkstra_dist, m->dijkstra_prev, m->search_visited);
        int cur = dst;
        while (cur != -1 && m->dijkstra_prev[cur] != src && m->dijkstra_prev[cur] != -1)
            cur = m->dijkstra_prev[cur];
        if (cur != -1 && m->dijkstra_prev[cur] == src)
            next_vertex = cur;

    } else if (ghost_idx == 1 && !g->vulnerable) {
        /* PINKY: usa BFS */
        bfs(m->maze_graph, src, m->bfs_dist, m->bfs_prev);
        int cur = dst;
        while (cur != -1 && m->bfs_prev[cur] != src && m->bfs_prev[cur] != -1)
            cur = m->bfs_prev[cur];
        if (cur != -1 && m->bfs_prev[cur] == src)
            next_vertex = cur;

    } else if (ghost_idx == 2 && !g->vulnerable) {
        /* INKY: usa DFS — dá um caminho válido qualquer, propositalmente menos eficiente que BFS/Dijkstra para comparar algoritmos */
        int *visited = xcalloc(MAP_W * MAP_H, sizeof(int), "dfs_visited");
        dfs(m->maze_graph, src, dst, visited, m->dfs_path, &m->dfs_path_len);
        free(visited);
        if (m->dfs_path_len >= 2)
            next_vertex = m->dfs_path[1];
    }

    if (next_vertex >= 0) {
        int nx, ny;
        model_vertex_to_coord(next_vertex, &nx, &ny);
        *out_dx = nx - g->e.x;
        *out_dy = ny - g->e.y;

        /* Corrige wrap do tunel */
        if (*out_dx > 1) *out_dx = -1;
        if (*out_dx < -1) *out_dx = 1;
        return;
    }

    /* Fallback: Manhattan simples (Clyde e vulneraveis) */
    int dirs[4][2] = {{0,-1},{-1,0},{0,1},{1,0}};
    int best_d = g->vulnerable ? -1 : INF;
    *out_dx = g->e.dx; *out_dy = g->e.dy;

    for (int i = 0; i < 4; i++) {
        int dx = dirs[i][0], dy = dirs[i][1];
        if (dx == -g->e.dx && dy == -g->e.dy) continue;
        int nx = g->e.x + dx, ny = g->e.y + dy;
        if (!model_is_walkable(m, nx, ny, 1)) continue;
        int dist = manhattan((nx + MAP_W) % MAP_W, ny, tx, ty);

        if (!g->vulnerable && dist < best_d) {
            best_d = dist; *out_dx = dx; *out_dy = dy;
        } else if (g->vulnerable && dist > best_d) {
            best_d = dist; *out_dx = dx; *out_dy = dy;
        }
    }

    /* Fallback final */
    if (!model_is_walkable(m, g->e.x + *out_dx, g->e.y + *out_dy, 1)) {
        for (int i = 0; i < 4; i++) {
            if (model_is_walkable(m, g->e.x + dirs[i][0], g->e.y + dirs[i][1], 1)) {
                *out_dx = dirs[i][0]; *out_dy = dirs[i][1]; break;
            }
        }
    }
}

void model_move_ghosts(GameModel *m) {
    /* Shell sort nos fantasmas por distancia ao Pac-Man (para prioridade de update) */
    int order[GHOSTS] = {0, 1, 2, 3};
    int dists[GHOSTS];
    for (int i = 0; i < GHOSTS; i++)
        dists[i] = manhattan(m->ghosts[i].e.x, m->ghosts[i].e.y, m->pacman.x, m->pacman.y);

    /* Shell sort por distancia */
    for (int gap = GHOSTS / 2; gap > 0; gap /= 2) {
        for (int i = gap; i < GHOSTS; i++) {
            int tmp_o = order[i], tmp_d = dists[i], j;
            for (j = i; j >= gap && dists[j - gap] > tmp_d; j -= gap) {
                order[j] = order[j - gap];
                dists[j] = dists[j - gap];
            }
            order[j] = tmp_o;
            dists[j] = tmp_d;
        }
    }

    for (int idx = 0; idx < GHOSTS; idx++) {
        int i = order[idx];
        Ghost *g = &m->ghosts[i];

        /* Fantasma na ghost house: espera timer */
        if (g->in_house) {
            if (g->release_timer > 0) { g->release_timer--; continue; }
            /* Sai da ghost house */
            g->e.x = 14; g->e.y = 11;
            g->e.dx = (rand() % 2) ? 1 : -1;
            g->e.dy = 0;
            g->in_house = 0;
            continue;
        }

        /* Vulneraveis se movem mais devagar */
        if (g->vulnerable && m->frame_count % 2 != 0) continue;

        int tx = m->pacman.x, ty = m->pacman.y;

        /* Targeting diferenciado */
        switch (i) {
            case 0: break; /* Blinky: direto */
            case 1: /* Pinky: 4 tiles a frente */
                tx += m->current_dx * 4;
                ty += m->current_dy * 4;
                break;
            case 2: /* Inky: reflexo de Blinky */
                tx = m->pacman.x + m->current_dx * 2;
                ty = m->pacman.y + m->current_dy * 2;
                tx = 2 * tx - m->ghosts[0].e.x;
                ty = 2 * ty - m->ghosts[0].e.y;
                break;
            case 3: /* Clyde: foge se perto */
                if (manhattan(g->e.x, g->e.y, m->pacman.x, m->pacman.y) < 8) {
                    tx = 0; ty = MAP_H - 1;
                }
                break;
        }

        if (tx < 0) tx = 0;
        if (tx >= MAP_W) tx = MAP_W - 1;
        if (ty < 0) ty = 0;
        if (ty >= MAP_H) ty = MAP_H - 1;

        int ndx, ndy;
        ghost_get_next_dir(m, i, tx, ty, &ndx, &ndy);
        g->e.dx = ndx;
        g->e.dy = ndy;

        if (model_is_walkable(m, g->e.x + g->e.dx, g->e.y + g->e.dy, 1)) {
            g->e.x += g->e.dx;
            g->e.y += g->e.dy;
            wrap_entity(&g->e);
        }
    }
}

int model_check_collisions(GameModel *m) {
    for (int i = 0; i < GHOSTS; i++) {
        if (m->ghosts[i].e.x == m->pacman.x && m->ghosts[i].e.y == m->pacman.y) {
            if (m->ghosts[i].vulnerable) {
                m->ghost_eat_combo++;
                m->score += 200 * m->ghost_eat_combo;
                if (m->score > m->high_score) m->high_score = m->score;

                m->ghosts[i].e.x = m->ghosts[i].e.start_x;
                m->ghosts[i].e.y = m->ghosts[i].e.start_y;
                m->ghosts[i].vulnerable = 0;
                m->ghosts[i].eaten = 1;
                m->ghosts[i].in_house = 1;
                m->ghosts[i].release_timer = 15;
                
                /* Usa Dijkstra para calcular o caminho mais rápido de volta pra casa */
                int ghost_home = model_coord_to_vertex(m->ghosts[i].e.start_x, m->ghosts[i].e.start_y);
                dijkstra(m->maze_graph, ghost_home, m->dijkstra_dist, m->dijkstra_prev, m->search_visited);
                
                return 200 * m->ghost_eat_combo;  /* Retorna bonus para exibir */
            } else {
                m->lives--;
                if (m->lives <= 0) {
                    m->state = STATE_GAMEOVER;

                    model_register_final_score(m);
                } else {
                    m->state = STATE_DYING;
                    m->death_anim = 0;
                }
                return -1;
            }
        }
    }
    return 0;
}


void model_load_scores(GameModel *m) {
    /* Complexidade O(n log n) media: carrega n scores e insere na BST. */
    bst_load_binary(&m->score_tree, SCORE_FILE);
}

void model_save_scores(GameModel *m) {
    /* Complexidade O(n): persiste os high scores da BST em arquivo binario. */
    bst_save_binary(m->score_tree, SCORE_FILE);
}

void model_register_final_score(GameModel *m) {
    /* Complexidade O(h): insere score final na BST e remove minimo se passar do limite. */
    if (m->score <= 0) return;
    m->score_tree = bst_insert(m->score_tree, m->score, m->player_name[0] ? m->player_name : "BRUXO");
    BSTNode *found = bst_search(m->score_tree, m->score);
    m->bst_last_search = found ? found->score : -1;
    while (bst_count(m->score_tree) > MAX_SCORES) {
        BSTNode *mn = bst_min(m->score_tree);
        if (!mn) break;
        m->score_tree = bst_remove(m->score_tree, mn->score);
    }
    BSTNode *mx = bst_max(m->score_tree);
    if (mx) m->high_score = mx->score;
    model_save_scores(m);
}

void model_print_final_report(GameModel *m) {
    printf("\nRELATORIO FINAL DE HOGWARTS\n");
    printf("Jogador: %s | Score: %d\n", m->player_name, m->score);
    printf("BST: altura=%d, nos=%d", bst_height(m->score_tree), bst_count(m->score_tree));
    BSTNode *mn = bst_min(m->score_tree), *mx = bst_max(m->score_tree);
    if (mn && mx) printf(", min=%d, max=%d", mn->score, mx->score);
    printf("\nAVL: rotacoes simples=%d, duplas=%d, fator raiz=%d\n", avl_rotation_single_count(), avl_rotation_double_count(), avl_balance_factor(m->powerup_tree));
    long long best = 0; const char *names[7] = {"Bubble","Selection","Insertion","Shell","Merge","Quick","Heap"}; int idx = -1;
    for (int i=0;i<7;i++) if (m->sort_times[i] > 0 && (best == 0 || m->sort_times[i] < best)) { best = m->sort_times[i]; idx = i; }
    printf("Sort mais rapido: %s (%lld us)\n", idx >= 0 ? names[idx] : "-", best);
}

void model_free(GameModel *m) {
    bst_free(m->score_tree);
    avl_free(m->powerup_tree);
    if (m->maze_graph) graph_free(m->maze_graph);
    m->score_tree = NULL;
    m->powerup_tree = NULL;
    m->maze_graph = NULL;
}
